#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <mpi.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <stddef.h>

#include "phoenix.h"
#include "px_checkpoint.h"
#include "px_log.h"
#include "px_read.h"
#include "px_remote.h"
#include "px_debug.h"
#include "px_util.h"
#include "px_allocate.h"
#include "px_constants.h"
#include "px_sampler.h"
#include "timecount.h"
#include "px_threadpool.h"
#include "px_destager.h"
#include "px_earlycopy.h"

#define CONFIG_FILE_NAME "phoenix.config"

//config file params
#define NVM_SIZE "nvm.size"
#define CHUNK_SIZE "chunk.size"
#define COPY_STRATEGY "copy.strategy"
#define DEBUG_ENABLE "debug.enable"
#define PFILE_LOCATION "pfile.location"
#define NVRAM_WBW "nvram.wbw"
#define RSTART "rstart"
#define REMOTE_CHECKPOINT_ENABLE "rmt.chkpt.enable"
#define REMOTE_RESTART_ENABLE "rmt.rstart.enable"
#define BUDDY_OFFSET "buddy.offset"
#define SPLIT_RATIO "split.ratio"
#define CR_TYPE "cr.type"
#define FREE_MEMORY "free.memory"
#define THRESHOLD_SIZE "threshold.size"
#define MAX_CHECKPOINTS "max.checkpoints"
#define EARLY_COPY_ENABLED "early.copy.enabled"
#define EARLY_COPY_OFFSET "early.copy.offset"  // (microseconds) when early copy invalidated. we increase the early copy time by this amount


//copy stategies
#define NAIVE_COPY 1
/*#define FAULT_COPY 2
#define PRE_COPY 3
#define PAGE_ALIGNED_COPY 4
#define REMOTE_COPY 5
#define REMOTE_FAULT_COPY 6
#define REMOTE_PRE_COPY 7*/


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int is_remaining_space_enough(int);


/*default config params*/
long log_size = 2*1024*1024;
int chunk_size = 4096;
int copy_strategy = 1;
int lib_initialized = 0;

int lib_process_id = -1;
int  checkpoint_size_printed = 0; // flag variable
long nvram_checkpoint_size = 0;
long local_dram_checkpoint_size =0;
long remote_dram_checkpoint_size=0;
int nvram_wbw = -1;
int rstart = 0;
int remote_checkpoint = 0;
int remote_restart = 0;
char pfile_location[32];
int n_processes = -1;  // number of processes assigned for this MPI job
int buddy_offset = 1;  // offset used during buddy checkpointing.
int split_ratio = 0;  // controls what portions of variables get checkpointed NVRAM vs DRAM
struct timeval px_start_time;
int cr_type = TRADITIONAL_CR;  // a flag for traditional checkpoint restart and online checkpoint restart usage
long checkpoint_iteration = 1; // keeping track of iterations, for running sampling
long free_memory = -1; // the memory budget passed as program config for derived from runtime
int threshold_size = 4096; // threshold size when deciding on moving to DRAM
long max_checkpoints = -1; // termination checkpoint iteration.
threadpool_t *thread_pool; // threadpool for destaging and earlycopy
long checkpoint_version; // keeping track of the latest checkpoint version
int early_copy_enabled = 0; // enable disable early copy
struct timeval px_lchk_time; // track the last checkpoint time, get used during early copy sleeps
volatile long early_copy_offset_add = 0; // increase early copy time if invalidated

destage_t targ1;
earlycopy_t targ2;

var_t *varmap = NULL;


int status; // error status register

log_t chlog;
dlog_t chdlog;

//semaphores used for synchronization between main thread and the early copy thread
static sem_t sem1;
static sem_t sem2;

static void early_copy_handler(int sig, siginfo_t *si, void *unused);

int init(int proc_id, int nproc){
    gettimeofday(&px_start_time,NULL);
	char varname[30];
	char varvalue[32];// we are reading integers in to this
	if(lib_initialized){
		printf("Error: the library already initialized.");
		exit(1);
	}

	lib_process_id = proc_id;
    n_processes = nproc;

	
	//naive way of reading the config file
	FILE *fp = fopen(CONFIG_FILE_NAME,"r");
	if(fp == NULL){
		printf("error while opening the file\n");
		exit(1);
	}
	while (fscanf(fp, "%s =  %s", varname, varvalue) != EOF) { // TODO: space truncate
        if (varname[0] == '#') {
            // consuming the input line starting with '#'
            fscanf(fp, "%*[^\n]");
            fscanf(fp, "%*1[\n]");
            continue;
        } else if (!strncmp(NVM_SIZE, varname, sizeof(varname))) {
            log_size = atoi(varvalue) * 1024 * 1024;
        } else if (!strncmp(CHUNK_SIZE, varname, sizeof(varname))) {
            chunk_size = atoi(varvalue);
        } else if (!strncmp(COPY_STRATEGY, varname, sizeof(varname))) {
            copy_strategy = atoi(varvalue);
        } else if (!strncmp(DEBUG_ENABLE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 1) {
                enable_debug();
            }
        } else if (!strncmp(PFILE_LOCATION, varname, sizeof(varname))) {
            strncpy(pfile_location, varvalue, sizeof(pfile_location));
        } else if (!strncmp(NVRAM_WBW, varname, sizeof(varname))) {
            nvram_wbw = atoi(varvalue);
        } else if (!strncmp(RSTART, varname, sizeof(varname))) {
            rstart = atoi(varvalue);
        } else if (!strncmp(REMOTE_CHECKPOINT_ENABLE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 1) {
                remote_checkpoint = 1;
            }
        } else if (!strncmp(REMOTE_RESTART_ENABLE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 1) {
                remote_restart = 1;
            }
        } else if (!strncmp(BUDDY_OFFSET, varname, sizeof(varname))) {
            buddy_offset = atoi(varvalue);
        } else if (!strncmp(SPLIT_RATIO, varname, sizeof(varname))) {
            split_ratio = atoi(varvalue);
        } else if (!strncmp(CR_TYPE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 0) {
                cr_type = TRADITIONAL_CR;
                if (lib_process_id == 0) {
                    log_info("traditional C/R mode. Single DRAM checkpoints\n");
                }
            } else if (atoi(varvalue) == 1) {
                cr_type = ONLINE_CR;
                if (lib_process_id == 0) {
                    log_info("Online C/R mode. Double checkpointing enabled\n");
                }
            }
        } else if (!strncmp(FREE_MEMORY, varname, sizeof(varname))) {
            free_memory = atol(varvalue) * 1024 * 1024;
        } else if (!strncmp(THRESHOLD_SIZE, varname, sizeof(varname))) {
            threshold_size = atoi(varvalue);
        } else if (!strncmp(MAX_CHECKPOINTS, varname, sizeof(varname))){
            max_checkpoints = atol(varvalue);
        }else if (!strncmp(EARLY_COPY_ENABLED, varname, sizeof(varname))){
            early_copy_enabled = atoi(varvalue);
        } else if (!strncmp(EARLY_COPY_OFFSET, varname, sizeof(varname))){
            early_copy_offset_add = atol(varvalue);
        } else{
			log_err("unknown varibale : %s  please check the config",varname);
			exit(1);
		}
	}
	fclose(fp);	
	if(isDebugEnabled()){
		printf("size of log in bytes : %ld bytes\n", log_size);
		printf("chunk size in bytes : %d\n", chunk_size);
		printf("copy strategy is set to : %d\n", copy_strategy);
		printf("persistant file location : %s\n", pfile_location);
		printf("NVRAM write bandwidth : %d Mb\n", nvram_wbw);
	}
    status = remote_init(proc_id,nproc);
    if(status){printf("Error: initializing remote copy procedures..\n");}

	log_init(&chlog,log_size,proc_id);
    dlog_init(&chdlog);


    if(proc_id == 0){
        start_memory_sampling_thread(); // sampling free DRAM memory during first checkpoint cycle
        debug("start memory sampling thread\n");
    }

    //creating threadpool for earlycopy and destage
    //all the threads should run in a single dedicated core/ or two.
    int THREAD_COUNT = 2;
    int QUEUE_SIZE = 2;
    thread_pool = threadpool_create(THREAD_COUNT,QUEUE_SIZE,3);


    //initializaing signaling semaphores. share with threads, init to '0'
    if(sem_init(&sem1,0,0) == -1){
        log_err("[%d] init semaphore one",lib_process_id);
    }
    if(sem_init(&sem2,0,0) == -1){
        log_err("[%d] init semaphore two",lib_process_id);
    }

    gettimeofday(&px_lchk_time,NULL);
    if(isDebugEnabled()){
        printf("phoenix initializing completed\n");
    }
	return 0;	
}


void *alloc_c(char *varname, size_t size, size_t commit_size,int process_id){
    var_t *s;
    varname = null_terminate(varname);
    if(is_chkpoint_present(&chlog)){
		if(isDebugEnabled()){
			printf("retrieving from the checkpointed memory : %s\n", varname);
		}
	/*Different copy methods*/
		switch(copy_strategy){
			case NAIVE_COPY:
				s = copy_read(&chlog, varname,process_id,checkpoint_version);
				break;
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}
	}else{
		if(isDebugEnabled()){
			printf("[%d] allocating from the heap space : %s\n",lib_process_id, varname);
		}
        s = px_alighned_allocate(size, process_id,varname);

	}
    s->type = NVRAM_CHECKPOINT; // default checkpoint location is NVRAM
    s->started_tracking = 0;
    s->end_timestamp = (struct timeval) {0,0};

    HASH_ADD_STR(varmap, varname, s );
    return s->ptr;
}






void chkpt_all(int process_id) {

    //starting from second iteration
    if (early_copy_enabled && checkpoint_iteration > 2) {
        //signal we are about to checkpoint
        //debug("[%d] wait on sem1",lib_process_id);
        if (sem_post(&sem1) == -1) {
            log_err("semaphore one increment");
            exit(-1);
        }
        //debug("[%d] acquired sem1",lib_process_id);
        //wait for the signal from early copy thread
        //debug("[%d] wait on sem2",lib_process_id);
        if (sem_wait(&sem2) == -1) {
            log_err("semaphore two wait");
        }
        //debug("[%d] acquired sem2",lib_process_id);
    }



    // if this is the max checkpoints, flush the timers and exit
    if(checkpoint_iteration == max_checkpoints){
        if(lib_process_id == 0){
            log_info("terminating after %ld checkpoints",checkpoint_iteration);
        }
        end_timestamp();
        MPI_Barrier(MPI_COMM_WORLD);
        exit(0);
        return;
    }

    if(rstart == 1){
        printf("skipping checkpointing data of process : %d \n",process_id);
        return;
    }


    //stop memory sampling thread after first iteration
    if(process_id == 0 && checkpoint_iteration == 1){
        flush_access_times();
        stop_memory_sampling_thread();
        start_page_tracking();
    }

    //get the access time value after second iteration
    if(process_id == 0 && checkpoint_iteration == 2){
        stop_page_tracking(); //tracking started during alloc() calls

    }

    if(checkpoint_iteration == 2){
        calc_early_copy_times(); //calculate early copy times
        broadcast_page_tracking(); // broadcast the page tracking details to other nodes
        install_sighandler(&early_copy_handler); //install early copy handler

    }

    if(checkpoint_iteration == 1){ // if this is first checkpoint of the app
        if(split_ratio >= 0){ //ratio based split
            if(lib_process_id == 0) {
                log_info("using config split ratio on choosing DRAM variables");
            }
            split_checkpoint_data(varmap);
        }else { // memory usage based split
            long long fmem = get_free_memory();
            if(free_memory != -1){
                fmem = free_memory ; // if there is a config value accept that
            }
            if(lib_process_id == 0){
                log_info("[%d] using memory access info to decide on DRAM variables",lib_process_id);
                log_info("[%d] free memory limit per process : %lld",lib_process_id, fmem);
            }
            decide_checkpoint_split(varmap, fmem);
        }
    }

    //aquire nvlog
    //debug("[%d] acquire nvlog lock", lib_process_id);
    if(pthread_mutex_lock(&mtx)){
        log_err("error acquiring lock");
    }

    /*checkpoint to local NVRAM, local DRAM and remote DRAM
     * we cannot do this parallely, no cores to run*/

    log_write(&chlog,varmap,process_id,checkpoint_version);//local NVRAM write

    if(pthread_mutex_unlock(&mtx)){
        log_err("error releasing lock");
    }


    int dlog_data=is_dlog_checkpoing_data_present(varmap);

    if(dlog_data) { // nvram checkpoint version updated by destaging thread
        //debug("[%d] dram checkpoint data present", process_id);
        dlog_local_write(&chdlog, varmap, process_id,checkpoint_version);//local DRAM write

        if (cr_type == ONLINE_CR) {
            dlog_remote_write(&chdlog, varmap, get_mypeer(process_id),checkpoint_version);//remote DRAM write
            //at this point we have a ONLINE_CR stable checkpoint
            chlog.current->head->online_version = checkpoint_version;
        }
    }else{ // pure NVRAM checkpoint
        chlog.current->head->current_version = checkpoint_version;
        //TODO: msync
    }

    if(lib_process_id == 0 && !checkpoint_size_printed){ // if this is the MPI main process log the checkpoint size
        printf("NVRAM checkpoint size : %.2f \n", (double)nvram_checkpoint_size/1000000);
        printf("local DRAM checkpoint size : %.2f \n", (double)local_dram_checkpoint_size/1000000);
        printf("remote DRAM checkpoint size : %.2f \n", (double)remote_dram_checkpoint_size/1000000);
        checkpoint_size_printed = 1;
    }

    gettimeofday(&px_lchk_time,NULL); // recording the last checkpoint time.


    //both destage and early copy will be done by the next checkpoint time.
    //hence we are recycling the structures.
    if(dlog_data) {
        //add destage task
        targ1.nvlog = &chlog;
        targ1.dlog = &chdlog;
        targ1.process_id = process_id;
        targ1.checkpoint_version = checkpoint_version;

        threadpool_add(thread_pool, &destage_data, (void *) &targ1, 0);
    }

    if(early_copy_enabled && checkpoint_iteration >= 2) {

        targ2.nvlog = &chlog;
        targ2.list = varmap;


        //add the precopy task
        threadpool_add(thread_pool, &start_copy, (void *) &targ2, 0);
    }
    //TODO : remove page protection done by early copy thread

    //debug("[%d] done with checkpoint iteration : %ld", lib_process_id,checkpoint_iteration);
    checkpoint_iteration++;
    checkpoint_version ++;



    return;
}




void *alloc(unsigned int *n, char *s, int *iid, int *cmtsize) {
	return alloc_c(s, *n, *cmtsize, *iid);
}

void afree_(void* ptr) {
	free(ptr);
}


void afree(void* ptr) {
	free(ptr);
}

void chkpt_all_(int *process_id){

	chkpt_all(*process_id);
}
int init_(int *proc_id, int *nproc){
	return init(*proc_id,*nproc);
}


int finalize(){
    //remove semaphores
    if(sem_destroy(&sem1) == -1){
        goto err;
    }
    if(sem_destroy(&sem2)== -1){
        goto err;
    }

    threadpool_destroy(thread_pool,threadpool_graceful);
    return remote_finalize();

    err:
        log_err("[%d] program error",lib_process_id);
        return -1;

}

int finalize_(){
    return finalize();
}






/*copy the data from local DRAM log to NVRAM log*/
void destage_data(void *args){
    destage_t *ds = (destage_t *)args;

    if(pthread_mutex_lock(&mtx)){
        log_err("error acquiring lock");
    }

    destage_data_log_write(ds->nvlog,ds->dlog->map[NVRAM_CHECKPOINT],ds->process_id);
    ds->nvlog->current->head->current_version = ds->checkpoint_version;


    if(pthread_mutex_unlock(&mtx)){
        log_err("error releasing lock");
    }

    /*if(msync(ds->nvlog->current->head,sizeof(headmeta_t),MS_SYNC) == -1){
        log_err("msync failed");
        exit(-1);
    }*/
    assert(ds->nvlog->current->head->online_version == ds->checkpoint_version); //double checkpoint stable pushed in to nvram as well
    debug("[%d] checkpoint data destaged." , lib_process_id);
    return;
}



/*
 * we install the early copyhandler after the first two iteration of the
 * checkpoint.
 *
 * 1.store the new access time
 * 2. invalidate the early copy variable
 * 3. remove page-protection
 *
 */
static void early_copy_handler(int sig, siginfo_t *si, void *unused){
    if(si != NULL && si->si_addr != NULL){
        var_t *s;
        void *pageptr;
        long offset =0;
        pageptr = si->si_addr;

        for(s = varmap; s != NULL; s = s->hh.next){
            offset = pageptr - s->ptr;
            if (offset >= 0 && offset <= s->size) { // the adress belong to this chunk.
                assert(s != NULL);
                log_info("[%d] early copy of %s , invalidated , offset %ld.%06ld",lib_process_id, s->varname,
                s->earlycopy_time_offset.tv_sec,s->earlycopy_time_offset.tv_usec);
                s->early_copied = 0;
                struct timeval inc;
                inc.tv_sec = 0;
                inc.tv_usec = early_copy_offset_add;
                timeradd(&s->earlycopy_time_offset,&inc,&s->earlycopy_time_offset);
                disable_protection(s->ptr, s->paligned_size);
                return;
            }
        }
        debug("[%d] offending memory access : %p ",lib_process_id,pageptr);
        call_oldhandler(sig);
    }

}




int ascending_time_sort(var_t *a, var_t *b){
    if(timercmp(&(a->end_timestamp),&(b->end_timestamp),<)){ // if a timestamp greater than b
        return -1;
    }else if(timercmp(&(a->end_timestamp),&(b->end_timestamp),==)){
        return 0;
    }else if(timercmp(&(a->end_timestamp),&(b->end_timestamp),>)){
        return 1;
    }else{
        assert("wrong execution path");
        exit(1);
    }
}

int sorted = 0;
void start_copy(void *args){
    earlycopy_t *ecargs = (earlycopy_t *)args;
    var_t *s;
    int sem_ret;
    //debug("early copy task started");

    //sort the access times
    if(!sorted) {
        HASH_SORT(varmap, ascending_time_sort);
        sorted = 1;
    }
    s = varmap;

    //aquire nvlog
    if(pthread_mutex_lock(&mtx)){
        log_err("error acquiring lock");
    }


    struct timeval current_time;
    struct timeval time_since_last_checkpoint;
    //check the signaling semaphore
    //debug("[%d] outside while loop",lib_process_id);
    while ((sem_ret = sem_trywait(&sem1)) == -1){
        if(s == NULL){
            sem_wait(&sem1); // TODO this is not the exact behaviour
            break;
        }
        //debug("[%d] outside while loop", lib_process_id);
        //main MPI process still hasnt reached the checkpoint!
        if(errno == EAGAIN){ // only when asynchronous wait fail
            /*int c = sched_getcpu();
            log_info("[%d] early copying variables, running on CPU - %d",lib_process_id,c);*/



            gettimeofday(&current_time,NULL);

            timersub(&current_time,&px_lchk_time,&time_since_last_checkpoint);
            //debug("[%d] variable : %s , offset time -  %ld.%06ld time since lchkpt - %ld.%06ld",
             //     lib_process_id,s->varname, s->earlycopy_time_offset.tv_sec, s->earlycopy_time_offset.tv_usec,
             //     time_since_last_checkpoint.tv_sec, time_since_last_checkpoint.tv_usec);
            //printf("sleeptime %ld.%06ld\n",sleeptime.tv_sec, sleeptime.tv_usec);*/

            // if the time is greater than variable, start copy
            if(timercmp(&time_since_last_checkpoint,&(s->earlycopy_time_offset),>)){

                if(s->type == NVRAM_CHECKPOINT) {


                    //page protect it. We disable the protection in a subsequent write or during checkpoint
                    // see log_write method
                    enable_write_protection(s->ptr, s->size);
                    //copy the variable
                    log_write_var(ecargs->nvlog, s, checkpoint_version);
                    //mark it as copied
                    s->early_copied = 1;
                    //log_info("[%d] early copied the variable : %s", lib_process_id, s->varname);
                }
                //move the iterator forward
                s = s->hh.next;
                continue;
            }
            else {

                if(s->type == DRAM_CHECKPOINT){
                    //debug("[%d] DRAM variable : %s",lib_process_id, s->varname);
                    s = s->hh.next;
                    continue;
                }
                //calculate sleep time
                //debug("[%d] early copy thread to sleep",lib_process_id);
                struct timeval sleeptime;
                timersub(&(s->earlycopy_time_offset), &time_since_last_checkpoint, &sleeptime);
                //
                uint64_t micros = (sleeptime.tv_sec * (uint64_t) 1000000) + (sleeptime.tv_usec);

                if (micros > 10) { // TODO check output of timersub
                    uint64_t sleep_offset = 0;
                    //debug("[%d] early copy thread sleeping  : %ld" , lib_process_id, micros+sleep_offset);
                    usleep(micros + sleep_offset);
                    /*printf("pagenode time  %ld.%06ld\n",pagenode->earlycopy_timestamp.tv_sec, pagenode->earlycopy_timestamp.tv_usec);
                    printf("time since last chkpoint %ld.%06ld\n",time_since_last_checkpoint.tv_sec, time_since_last_checkpoint.tv_usec);
                    printf("sleeptime %ld.%06ld\n",sleeptime.tv_sec, sleeptime.tv_usec);*/


                }

            }
        }else{
                assert(0);
        }
    }
    //relase nvlog
    if(pthread_mutex_unlock(&mtx)){
        log_err("error releasing lock");
    }

    //debug("[%d] semaphore wait return value : %d", lib_process_id,sem_ret);
    if(sem_post(&sem2) == -1){
        log_err("semaphore two increment");
        exit(-1);
    }
    //debug("[%d] early copy thread exiting. sem ret value : %d",lib_process_id,sem_ret);
    return;
}

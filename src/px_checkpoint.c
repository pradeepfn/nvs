#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <pthread.h>
#include <dirent.h>
#include <semaphore.h>
#include <unistd.h>

#include "phoenix.h"
#include "px_log.h"
#include "px_read.h"
#include "px_remote.h"
#include "px_debug.h"
#include "px_util.h"
#include "px_allocate.h"
#include "px_constants.h"
#include "px_sampler.h"
#include "timecount.h"
#include "px_earlycopy.h"
#include "px_timer.h"
#include "px_dlog.h"



/* set some of the variables on the stack */
rcontext_t runtime_context;
ccontext_t config_context;
var_t *varmap = NULL;
destage_t destage_arg;
earlycopy_t ec_arg;
log_t nvlog;
dlog_t dlog;

/* local variables */
int lib_initialized = 0;


//declare file pointers
#ifdef TIMING
    FILE *ef,*cf,*df,*itf;
    TIMER_DECLARE4(et,ct,dt,it); // declaring earycopy_timer, checkpoint_timer,destage_timer
#endif




static void early_copy_handler(int sig, siginfo_t *si, void *unused);
void destage_data(void *args);

int init(int proc_id, int nproc){
    int status;

    //tie up the global variable hieararchy
    runtime_context.config_context = &config_context;
    runtime_context.varmap = varmap;
    nvlog.runtime_context = &runtime_context;
    dlog.runtime_context = &runtime_context;


	if(lib_initialized){
		printf("Error: the library already initialized.");
		exit(1);
	}
	runtime_context.process_id = proc_id;
    runtime_context.nproc = nproc;
    runtime_context.ec_abort = 0;
    runtime_context.ec_finished = 0;

    status = pthread_mutex_init(&runtime_context.mtx,NULL);
    if(status != 0){
        log_err("mutex init error");
    }
    status = pthread_cond_init(&runtime_context.cond,NULL);
    if(status != 0){
        log_err("mutex init error");
    }


//if TIMING is defined , we create log files, so that we can output the timings to them
#ifdef TIMING
    char file_name[50];
    DIR* dir = opendir("stats");
    if(dir){
		snprintf(file_name,sizeof(file_name),"stats/earlycopy%d.log",proc_id);
		ef=fopen(file_name,"w");
		snprintf(file_name,sizeof(file_name),"stats/checkpoint%d.log",proc_id);
		cf=fopen(file_name,"w");
        snprintf(file_name,sizeof(file_name),"stats/destagetime%d.log",proc_id);
        df=fopen(file_name,"w");
        snprintf(file_name,sizeof(file_name),"stats/iterationtime%d.log",proc_id);
        itf=fopen(file_name,"w");
    }else{ // directory does not exist
        printf("Error: no stats directory found.\n\n");
		assert(0);
    }
#endif
    TIMER_START(it); // timer for checkpoint iterations

    read_configs(&config_context,CONFIG_FILE_NAME);

	if(isDebugEnabled()){
		printf("size of log in bytes : %ld bytes\n", config_context.log_size);
		printf("chunk size in bytes : %d\n", config_context.chunk_size);
		printf("copy strategy is set to : %d\n", config_context.copy_strategy);
		printf("persistant file location : %s\n", config_context.pfile_location);
		printf("NVRAM write bandwidth : %d Mb\n", config_context.nvram_wbw);
	}
    status = remote_init(proc_id,nproc,config_context.buddy_offset);
    if(status){printf("Error: initializing remote copy procedures..\n");}

	log_init(&nvlog,proc_id);
    dlog_init(&dlog);

    if(proc_id == 0){
        start_memory_sampling_thread(); // sampling free DRAM memory during first checkpoint cycle
        debug("start memory sampling thread\n");
    }
    //creating threadpool for earlycopy and destage
    //all the threads should run in a single dedicated core/ or two.
    int THREAD_COUNT = 2;
    int QUEUE_SIZE = 2;
    runtime_context.thread_pool = threadpool_create(THREAD_COUNT,QUEUE_SIZE,
                                                    config_context.helper_cores,config_context.helper_core_size);

    gettimeofday(&(runtime_context.px_start_time),NULL);
    if(isDebugEnabled()){
        printf("phoenix initializing completed\n");
    }
	return 0;	
}


void *alloc_c(char *varname, size_t size, size_t commit_size,int process_id){
    var_t *s;
    varname = null_terminate(varname);
    if(is_chkpoint_present(&nvlog)){
		if(isDebugEnabled()){
			printf("retrieving from the checkpointed memory : %s\n", varname);
		}
	/*Different copy methods*/
		switch(config_context.copy_strategy){
			case NAIVE_COPY:
				s = copy_read(&nvlog, varname,process_id,runtime_context.checkpoint_version);
				break;
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}
	}else{
		if(isDebugEnabled()){
			printf("[%d] allocating from the heap space : %s\n",runtime_context.process_id, varname);
		}
        s = px_alighned_allocate(size, process_id,varname);

	}
    s->type = NVRAM_CHECKPOINT; // default checkpoint location is NVRAM
    s->started_tracking = 0;
    s->end_timestamp = (struct timeval) {0,0};

    HASH_ADD_STR(varmap, varname, s );
    return s->ptr;
}





int checkpoint_size_printed=0;
void chkpt_all(int process_id) {

    gettimeofday(&(runtime_context.lchk_start_time),NULL); // recording the last checkpoint start time.

    ulong  it_elapsed = 0;
    TIMER_END(it,it_elapsed);
    #ifdef  TIMING
        fprintf(itf,"%lu\n",it_elapsed);
        fflush(itf);
    #endif

    TIMER_START(ct);
    //starting from second iteration
    if (config_context.early_copy_enabled && runtime_context.checkpoint_iteration > 2) {
        pthread_mutex_lock(&runtime_context.mtx);
        if(runtime_context.ec_abort != 0){
            log_err("invalid execution");
            exit(1);
        }
        runtime_context.ec_abort = 1;
        pthread_mutex_unlock(&runtime_context.mtx);
    }

    // if this is the max checkpoints, flush the timers and exit
    if(runtime_context.checkpoint_iteration == config_context.max_checkpoints){
        if(runtime_context.process_id == 0){
            log_info("terminating after %ld checkpoints",runtime_context.checkpoint_iteration);
        }
        end_timestamp();
        MPI_Barrier(MPI_COMM_WORLD);
        exit(0);
    }

    if(config_context.restart_run == 1){
        printf("skipping checkpointing data of process : %d \n",process_id);
        return;
    }


    //stop memory sampling thread after first iteration
    if(process_id == 0 && runtime_context.checkpoint_iteration == 1){
        stop_memory_sampling_thread();
        start_page_tracking();
    }

    //get the access time value after second iteration
    if(process_id == 0 && runtime_context.checkpoint_iteration == 2){
        stop_page_tracking(); //tracking started during alloc() calls
        flush_access_times();
    }

    if(runtime_context.checkpoint_iteration == 2){
        calc_early_copy_times(&runtime_context); //calculate early copy times
        broadcast_page_tracking(&runtime_context); // broadcast the page tracking details to other nodes
        install_sighandler(&early_copy_handler); //install early copy handler

    }

    if(runtime_context.checkpoint_iteration == 1){ // if this is first checkpoint of the app
        if(config_context.split_ratio >= 0){ //ratio based split
            if(runtime_context.process_id == 0) {
                log_info("using config split ratio on choosing DRAM variables");
            }
            split_checkpoint_data(&runtime_context, varmap);
        }else { // memory usage based split
            long long fmem = get_free_memory();
            if(config_context.free_memory != -1){
                fmem = config_context.free_memory ; // if there is a config value accept that
            }
            if(runtime_context.process_id == 0){
                log_info("[%d] using memory access info to decide on DRAM variables",runtime_context.process_id);
                log_info("[%d] free memory limit per process : %lld",runtime_context.process_id, fmem);
            }
            decide_checkpoint_split(&runtime_context, varmap, fmem);
        }
    }
    debug("[%d] NVRAM checkpoint start.. ",runtime_context.process_id);
    /*checkpoint to local NVRAM, local DRAM and remote DRAM
     * we cannot do this parallely, no cores to run*/
    var_t *s;
    int flag = 0;
    for (s = varmap; s != NULL; s = s->hh.next){

        pthread_mutex_lock(&runtime_context.mtx);
        debug("about to checkpoint");
        if(s->process_id == process_id && s->type == NVRAM_CHECKPOINT && (!s->early_copied) ){
            if(runtime_context.checkpoint_iteration >= 3) {
                s->early_copied = 1; // no checkpoint yet. I will checkpoint.
            }
            flag = 1; // have to checkpoint this variable
        }
        pthread_mutex_unlock(&runtime_context.mtx);
        if(flag) {
            runtime_context.nvram_checkpoint_size += s->size;
            log_write(&nvlog, s, process_id, runtime_context.checkpoint_version);
            flag = 0;
        }

    }

    //check if early copy thread exited. if not wait for it. we rarely have to wait on this.
    if(config_context.early_copy_enabled && runtime_context.checkpoint_iteration >=3) {
        int status = pthread_mutex_lock(&(runtime_context.mtx));
        if (status != 0) {
            log_err("error while acquiring lock");
            exit(status);
        }
        while (!runtime_context.ec_finished) {
            log_warn("waiting on early copy thread. This should not happen");
            status = pthread_cond_wait(&runtime_context.cond, &runtime_context.mtx);
            if (status != 0) {
                log_err("error while unlock");
                exit(status);
            }
        }
        runtime_context.ec_finished = 0; // resetting
        status = pthread_mutex_unlock(&runtime_context.mtx);
        if (status != 0) {
            log_err("error while unlock");
            exit(status);
        }

        // at this point only one thread executing
        for (s = varmap; s != NULL; s = s->hh.next) {
            disable_protection(s->ptr, s->size); // resetting the page protection
            s->early_copied = 0; // resetting the flag to next iteration
        }
        runtime_context.ec_abort = 0; // reset the flag for next iteration
    }//end of NVRAM checkpoints


    int dlog_data=is_dlog_checkpoing_data_present(varmap);

    if(dlog_data) { // nvram checkpoint version updated by destaging thread
        //debug("[%d] dram checkpoint data present", process_id);
        dlog_local_write(&dlog, varmap, process_id,runtime_context.checkpoint_version);//local DRAM write

        if (config_context.cr_type == ONLINE_CR) {
            dlog_remote_write(&dlog, varmap, get_mypeer(&runtime_context, process_id),runtime_context.checkpoint_version);//remote DRAM write
            //at this point we have a ONLINE_CR stable checkpoint
        }
    }else{ // pure NVRAM checkpoint
        log_commitv(&nvlog,runtime_context.checkpoint_version);
        //TODO: msync
    }

    if(runtime_context.process_id == 0 && !checkpoint_size_printed){ // if this is the MPI main process log the checkpoint size
        printf("NVRAM checkpoint size : %.2f \n", (double)runtime_context.nvram_checkpoint_size/1000000);
        printf("local DRAM checkpoint size : %.2f \n", (double)runtime_context.local_dram_checkpoint_size/1000000);
        printf("remote DRAM checkpoint size : %.2f \n", (double)runtime_context.remote_dram_checkpoint_size/1000000);
        checkpoint_size_printed = 1;
    }


    //both destage and early copy will be done by the next checkpoint time.
    //hence we are recycling the structures.
    if(dlog_data) {
        //add destage task
        destage_arg.nvlog = &nvlog;
        destage_arg.dlog = &dlog;
        destage_arg.process_id = process_id;
        destage_arg.checkpoint_version = runtime_context.checkpoint_version;

        threadpool_add(runtime_context.thread_pool, &destage_data, (void *) &destage_arg, 0);
    }
    timersub(&runtime_context.lchk_start_time,&runtime_context.lchk_end_time,&runtime_context.lchk_iteration_time);
    gettimeofday(&(runtime_context.lchk_end_time),NULL); // recording the last checkpoint end time little early. we feed this to early copy thread

    //scheduling early copy thread of the next iteration
    if(config_context.early_copy_enabled && runtime_context.checkpoint_iteration >= 2) {

        ec_arg.nvlog = &nvlog;
        ec_arg.list = varmap;
        ec_arg.runtime_context = &runtime_context;
        ec_arg.checkpoint_end_time = runtime_context.lchk_end_time;
        ec_arg.next_checkpoint_time = runtime_context.lchk_iteration_time;
        //add the precopy task
        threadpool_add(runtime_context.thread_pool, &start_copy, (void *) &ec_arg, 0);
    }

    //debug("[%d] done with checkpoint iteration : %ld", lib_process_id,checkpoint_iteration);
    runtime_context.checkpoint_iteration++;
    runtime_context.checkpoint_version ++;

    ulong  elapsed = 0;
    TIMER_END(ct,elapsed);
    #ifdef  TIMING
        fprintf(cf,"%lu\n",elapsed);
        fflush(cf);
    #endif
    TIMER_START(it);
    return;
}



/*copy the data from local DRAM log to NVRAM log
 * make priority to destaging over early copy thread
 */
void destage_data(void *args){
    destage_t *ds = (destage_t *)args;
    int status;
    TIMER_START(dt);

    var_t *s;
    for(s=ds->dlog->map[NVRAM_CHECKPOINT];s!=NULL;s=s->hh.next){
        status = log_write(ds->nvlog,s,runtime_context.process_id,ds->checkpoint_version);
        if(status == -1){
            log_err("nvlog write failed while data destage");
            exit(1);
        }
    }
    log_commitv(ds->nvlog,ds->checkpoint_version);
    ulong elapsed = 0;
    TIMER_END(dt,elapsed);
    #ifdef  TIMING
        fprintf(df,"%lu\n",elapsed);
        fflush(df);
    #endif
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
                debug("[%d] early copy of %s , invalidated , offset %lu.%lu",runtime_context.process_id, s->varname,
                      s->earlycopy_time_offset.tv_sec,s->earlycopy_time_offset.tv_usec);
                s->early_copied = 0;
                struct timeval inc;
                inc.tv_sec = 0;
                inc.tv_usec = config_context.ec_offset_add;
                timeradd(&s->earlycopy_time_offset,&inc,&s->earlycopy_time_offset);
                disable_protection(s->ptr, s->paligned_size);
                return;
            }
        }
        debug("[%d] offending memory access : %p ",runtime_context.process_id,pageptr);
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

    TIMER_START(et);
    earlycopy_t *ecargs = (earlycopy_t *)args;
    var_t *s;
    int sem_ret,status;
    //debug("early copy task started");

    //sort the access times
    if(!sorted) {
        HASH_SORT(varmap, ascending_time_sort);
        sorted = 1;
    }
    s = varmap;




    struct timeval current_time;
    struct timeval time_since_last_checkpoint;
    struct timeval time_till_next_checkpoint;

    TIMER_PAUSE(et);

    //check the signaling semaphore
    //debug("[%d] outside while loop",lib_process_id);
    pthread_mutex_lock(&runtime_context.mtx);
    while (runtime_context.ec_abort == 0 && s != NULL) {
        pthread_mutex_unlock(&runtime_context.mtx);
        //main MPI process still hasnt reached the checkpoint!
        gettimeofday(&current_time, NULL);
        timersub(&current_time, &(ecargs->checkpoint_end_time), &time_since_last_checkpoint);
        timersub(&(ecargs->next_checkpoint_time), &current_time, &time_till_next_checkpoint);

        // if the time is greater than variable, start copy
        if (timercmp(&time_since_last_checkpoint, &(s->earlycopy_time_offset), >)) {

            if (s->type == NVRAM_CHECKPOINT) {
                TIMER_RESUME(et);

                //page protect it. We disable the protection in a subsequent write or during checkpoint
                // see log_write method
                enable_write_protection(s->ptr, s->size);
                //mark it as copied. lock protected - accessed by the main thread as well.
                pthread_mutex_lock(&(ecargs->runtime_context->mtx));
                if (s->early_copied == 0) { //still not copied, i will take care of this
                    s->early_copied = 1;
                }
                pthread_mutex_unlock(&(ecargs->runtime_context->mtx));
                //copy the variable
                status = log_write(ecargs->nvlog, s, ecargs->runtime_context->process_id,
                                   ecargs->runtime_context->checkpoint_version);
                if (status == -1) {
                    log_err("early copying data to nvlog failed");
                }
                //log_info("[%d] early copied the variable : %s", lib_process_id, s->varname);
                TIMER_PAUSE(et);
            }
            //move the iterator forward
            s = s->hh.next;
            continue;

        } else {

            if (s->type == DRAM_CHECKPOINT) {
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

            //if sleep time is greater than time to checkpoint abort
            if (timercmp(&sleeptime, &time_till_next_checkpoint, >)) {
                break;
            }

            if (micros > 10) { // TODO check output of timersub
                uint64_t sleep_offset = 0;
                //debug("[%d] early copy thread sleeping  : %ld" , lib_process_id, micros+sleep_offset);
                usleep(micros + sleep_offset);
                /*printf("pagenode time  %ld.%06ld\n",pagenode->earlycopy_timestamp.tv_sec, pagenode->earlycopy_timestamp.tv_usec);
                printf("time since last chkpoint %ld.%06ld\n",time_since_last_checkpoint.tv_sec, time_since_last_checkpoint.tv_usec);
                printf("sleeptime %ld.%06ld\n",sleeptime.tv_sec, sleeptime.tv_usec);*/


            }

        }
    }
    pthread_mutex_unlock(&runtime_context.mtx);
    TIMER_RESUME(et);

    pthread_mutex_lock(&(ecargs->runtime_context->mtx));
        ecargs->runtime_context->ec_finished = 1;
    pthread_mutex_unlock(&(ecargs->runtime_context->mtx));
    pthread_cond_signal(&(ecargs->runtime_context->cond));

    ulong elapsed = 0;
    TIMER_END(et,elapsed);
    #ifdef  TIMING
        fprintf(ef,"%lu\n",elapsed);
        fflush(ef);
    #endif

    //debug("[%d] early copy thread exiting. sem ret value : %d",lib_process_id,sem_ret);
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

    pthread_mutex_destroy(&runtime_context.mtx);
    pthread_cond_destroy(&runtime_context.cond);


    ulong  it_elapsed = 0;
    TIMER_END(it,it_elapsed);
#ifdef  TIMING
    fprintf(itf,"%lu\n",it_elapsed);
    fflush(itf);
#endif

    //close file pointers
#ifdef TIMING
    fclose(ef);
    fclose(cf);
    fclose(df);
#endif

    threadpool_destroy(runtime_context.thread_pool,threadpool_graceful);
    return remote_finalize();

    /*err:
    log_err("[%d] program error",runtime_context.process_id);
    return -1;
*/

}

int finalize_(){
    return finalize();
}





#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <mpi.h>
#include <semaphore.h>
#include <unistd.h>
#include <sched.h>

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


//copy stategies
#define NAIVE_COPY 1
#define FAULT_COPY 2
#define PRE_COPY 3
#define PAGE_ALIGNED_COPY 4
#define REMOTE_COPY 5
#define REMOTE_FAULT_COPY 6
#define REMOTE_PRE_COPY 7


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
threadpool_t *thread_pool;

int status; // error status register

log_t chlog;
dlog_t chdlog;
listhead_t head;
tlisthead_t thead;

thread_t thread;

//semaphores used for synchronization between main thread and the early copy thread
static sem_t sem1;
static sem_t sem2;


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
        }else{
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
	LIST_INIT(&head);

    if(proc_id == 0){
        start_memory_sampling_thread(); // sampling free DRAM memory during first checkpoint cycle
        debug("start memory sampling thread\n");
    }

    //creating threadpool for earlycopy and destage
    //all the threads should run in a single dedicated core/ or two.
    int THREAD_COUNT = 2;
    int QUEUE_SIZE = 2;
    thread_pool = threadpool_create(THREAD_COUNT,QUEUE_SIZE,0);


    //initializaing signaling semaphores. share with threads, init to '0'
    if(sem_init(&sem1,0,0) == -1){
        log_err("[%d] init semaphore one",lib_process_id);
    }
    if(sem_init(&sem2,0,0) == -1){
        log_err("[%d] init semaphore two",lib_process_id);
    }


    if(isDebugEnabled()){
        printf("phoenix initializing completed\n");
    }
	return 0;	
}


void *alloc_c(char *var_name, size_t size, size_t commit_size,int process_id){

    entry_t *n = malloc(sizeof(struct entry)); // new node in list
    var_name = null_terminate(var_name);
    memcpy(n->var_name,var_name,VAR_SIZE);
    n->size = size;
    n->process_id = process_id;
    n->version = 0;

	//if checkpoint data present, then read from the the checkpoint
	if(remote_restart){

		//if(isDebugEnabled()){
		//	printf("retrieving from the remote checkpointed memory : %s\n", var_name);
		//}
		switch(copy_strategy){
		   	case REMOTE_COPY:	
				remote_copy_read(&chlog, var_name,get_mypeer(process_id),n);
				break;
			case REMOTE_FAULT_COPY:	
				remote_chunk_read(&chlog, var_name,get_mypeer(process_id),n);
				break;
			case REMOTE_PRE_COPY:	
				remote_pc_read(&chlog, var_name,get_mypeer(process_id),n);
				break;
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}

	}else if(is_chkpoint_present(&chlog)){
		if(isDebugEnabled()){
			printf("retrieving from the checkpointed memory : %s\n", var_name);
		}
	/*Different copy methods*/
		switch(copy_strategy){
			case NAIVE_COPY:
				n->ptr = copy_read(&chlog, var_name,process_id);
				break;
			case FAULT_COPY:
				n->ptr = chunk_read(&chlog, var_name,process_id);
				break;	
			case PRE_COPY:
				n->ptr = pc_read(&chlog, var_name,process_id);
				break;
			case PAGE_ALIGNED_COPY:	
				n->ptr = page_aligned_copy_read(&chlog, var_name,process_id);
				break;
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}
	}else{
		if(isDebugEnabled()){
			printf("[%d] allocating from the heap space : %s\n",lib_process_id,var_name);
		}
        n->ptr = px_alighned_allocate(size,var_name);
        n->type = NVRAM_CHECKPOINT; // default checkpoint location is NVRAM
	}
    LIST_INSERT_HEAD(&head, n, entries);
    return n->ptr;
}

typedef enum {
    LOCAL_NVRAM_WRITE,
    LOCAL_DRAM_WRITE,
}func_type;

/*typedef struct thread_data_t_{
    log_t *chlog;
    dlog_t *chdlog;
    int process_id;
    listhead_t *head;
    func_type type;
} thread_data_t;*/

/*void *thread_work(void *ptr){
    thread_data_t *t_data = (thread_data_t *)ptr;
    if(t_data -> type == LOCAL_NVRAM_WRITE){
        log_write(t_data->chlog,t_data->head,t_data->process_id);//local NVRAM write
    }else if(t_data->type == LOCAL_DRAM_WRITE){
        dlog_local_write(t_data->chdlog,t_data->head,t_data->process_id);//local DRAM write
    } else{
        assert( 0 && "wrong exection path");
    }
    pthread_exit((void*)t_data->type);
}*/

/*
 * The procedure is responsible for hybrid checkpoints. The phoenix version of local checkpoitns.
 * 1. First we mark what to checkpoint to local NVRAM/what goes to remote DRAM
 * 2. Parallelly starts;
 * 			- local NVRAM checkpoint - X portions of the Total checkpoint
 * 			- local DRAM checkpoint - (Total -X) portion of data
 * 			- remote DRAM checkpoint - (Total-X) portion of data
 * 	3. We are done with
 */



void chkpt_all(int process_id){

    //signal we are about to checkpoint
    if(sem_post(&sem1) == -1){
        log_err("semaphore one increment");
        exit(-1);
    }
    //wait for the signal from early copy thread
    if (sem_wait(&sem2) == -1){
        log_err("semaphore two wait");
    }

    /*// TODO: FIXME: fix this with a thread pool
    int NUM_THREADS = 2;
    pthread_t thread[NUM_THREADS];
    pthread_attr_t attr;
    int t,rc;
    void * sts;*/

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


    //stop memory sampling thread and stop page tracking
    if(process_id == 0 && checkpoint_iteration == 1){
        flush_access_times();
        stop_memory_sampling_thread();
        stop_page_tracking(); //tracking started during alloc() calls
    }

    if(checkpoint_iteration == 1){ // if this is first checkpoint of the app
        if(split_ratio >= 0){ //ratio based split
            if(lib_process_id == 0) {
                log_info("using config split ratio on choosing DRAM variables");
            }
            split_checkpoint_data(&head);
        }else { // memory usage based split
            long long fmem = get_free_memory();
            if(free_memory != -1){
                fmem = free_memory ; // if there is a config value accept that
            }
            if(lib_process_id == 0){
                log_info("[%d] using memory access info to decide on DRAM variables",lib_process_id);
                log_info("[%d] free memory limit per process : %lld",lib_process_id, fmem);
            }
            decide_checkpoint_split(&head, fmem);
        }
    }

    /* Initialize and set thread detached attribute *//*
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    thread_data_t *thread_data[2];
    for(t=0; t < NUM_THREADS; t++) {
        thread_data[t] = (thread_data_t *)malloc(sizeof(thread_data_t)) ;
        thread_data[t]->chlog = &chlog;
        thread_data[t]->chdlog = &chdlog;
        thread_data[t]->head = &head;
        thread_data[t]->process_id = process_id;
        thread_data[t]->type = t; // using integer for enum type

        if(isDebugEnabled()){
            printf("Main: creating thread %d\n", t);
        }
        rc = pthread_create(&thread[t], &attr, thread_work, (void *)thread_data[t]);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }*/

    /*checkpoint to local NVRAM, local DRAM and remote DRAM
     * we cannot do this parallely, no cores to run*/

    log_write(&chlog,&head,process_id);//local NVRAM write
    dlog_local_write(&chdlog,&head,process_id);//local DRAM write

    if(cr_type == ONLINE_CR){
        dlog_remote_write(&chdlog,&head,get_mypeer(process_id));//remote DRAM write
    }

    if(lib_process_id == 0 && !checkpoint_size_printed){ // if this is the MPI main process log the checkpoint size
        printf("NVRAM checkpoint size : %.2f \n", (double)nvram_checkpoint_size/1000000);
        printf("local DRAM checkpoint size : %.2f \n", (double)local_dram_checkpoint_size/1000000);
        printf("remote DRAM checkpoint size : %.2f \n", (double)remote_dram_checkpoint_size/1000000);
        checkpoint_size_printed = 1;
    }
    checkpoint_iteration++;

    //add destage task
    destage_t targ;
    targ.nvlog = &chlog;
    targ.dlog = &chdlog;
    targ.process_id = process_id;

    threadpool_add(thread_pool,&destage_data,(void *) &targ,0);
    //add the precopy task
    threadpool_add(thread_pool,&start_copy,NULL,0);

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
    destage_data_log_write(ds->nvlog,ds->dlog->map[NVRAM_CHECKPOINT],ds->process_id);
    return;
}



void start_copy(void *args){
    earlycopy_t *ec = (earlycopy_t *)args;
    int sem_ret;
    //check the signaling semaphore
    while ((sem_ret = sem_trywait(&sem1)) == -1){
        //main MPI process still hasnt reached the checkpoint!
        if(errno == EAGAIN){ // only when asynchronous wait fail
            int c = sched_getcpu();
            log_err("[%d] early copying variables, running on CPU - %d",lib_process_id,c);

            sleep(1);
        }else{
            assert(0); // wrong execution path
        }
    }

    if(sem_ret == 0 && sem_post(&sem2) == -1){
        log_err("semaphore two increment");
        exit(-1);
    }
    return;
}

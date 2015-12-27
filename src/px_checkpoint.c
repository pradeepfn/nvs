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
    runtime_context.varmap = &varmap;
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
    //stop memory sampling thread after first iteration
    if(process_id == 0 && runtime_context.checkpoint_iteration == 1){
        start_page_tracking();
    }



    if(process_id == 0 && runtime_context.checkpoint_iteration >1 && runtime_context.checkpoint_iteration <=3){
        flush_access_times();
        reset_trackers();
    }

    //get the access time value after second iteration
    if(process_id == 0 && runtime_context.checkpoint_iteration == 3){
        stop_page_tracking(); //tracking started during alloc() calls
    }

    runtime_context.checkpoint_iteration++;
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
    return 0;
}

int finalize_(){
    return finalize();
}





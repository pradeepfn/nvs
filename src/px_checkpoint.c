#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <pthread.h>
#include <dirent.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

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

FILE *fp_ptr;
//declare file pointers
#ifdef TIMING
FILE *ef,*cf,*df,*itf;
TIMER_DECLARE4(et,ct,dt,it); // declaring earycopy_timer, checkpoint_timer,destage_timer
#endif




static void early_copy_handler(int sig, siginfo_t *si, void *unused);
void destage_data(void *args);

int px_init(int proc_id, int nproc){
    debug("initializing phx with my proc : %d and nproc : %d",proc_id, nproc);	
	int status;
    char file_name[50];

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
	runtime_context.total_pages = 0;
	runtime_context.mod_pages=0;
	runtime_context.umod_pages=0;

	//state file
    DIR* dir = opendir("stats");
    if(dir) {
        snprintf(file_name, sizeof(file_name), "stats/pt%d.log",proc_id);
        fp_ptr = fopen(file_name, "w+");
    }else{ // directory does not exist
        printf("Error: no stats directory found.\n\n");
        exit(1);
    }
    debug("exiting init routine of phx");	
	return 0;
}

void flush_access_times(rcontext_t *rctxt){
        fprintf(fp_ptr,"%lu, %lu, %lu\n",rctxt->total_pages,rctxt->mod_pages, rctxt->umod_pages);
        fflush(fp_ptr);
}



void *alloc_c(char *varname, size_t size, size_t commit_size, int process_id){
	var_t *s;
	varname = null_terminate(varname); // no need for c programs, fix this
	assert(runtime_context.process_id == process_id);
	if(isDebugEnabled()){
		printf("[%d] allocating from the heap space : %s\n",runtime_context.process_id, varname);
	}
	s = px_alighned_allocate(size, process_id,varname);
	int page_size = sysconf(_SC_PAGESIZE);

	int npages = s->paligned_size / page_size;
	runtime_context.total_pages += npages;

	s->type = NVRAM_CHECKPOINT; // default checkpoint location is NVRAM
	s->started_tracking = 0;
	s->end_timestamp = (struct timeval) {0,0};

	HASH_ADD_STR(varmap, varname, s );
	return s->ptr;
}


void app_snapshot() {
	debug("creating app snapshot %d", runtime_context.process_id);
	//collect stats and print them to file
	runtime_context.umod_pages = runtime_context.total_pages - runtime_context.mod_pages;
	flush_access_times(&runtime_context);
	//reset modified counter
	runtime_context.mod_pages=0;
	//write protect pages and reset counters
	start_page_tracking();
	debug("done creating snapshot %d", runtime_context.process_id);
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

void app_snapshot_(){
	app_snapshot();
}
int init_(int *proc_id, int *nproc){
	return px_init(*proc_id,*nproc);
}


int px_finalize(){
	stop_page_tracking(); //tracking started during alloc() calls
	fclose(fp_ptr);
	return 0;
}

int finalize_(){
	return px_finalize();
}





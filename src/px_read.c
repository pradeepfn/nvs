#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#include "px_log.h"
#include "px_checkpoint.h"
#include "px_debug.h"
#include "px_util.h"
#include "px_read.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void bchandler(int sig, siginfo_t *si, void *unused);
static void pchandler(int sig, siginfo_t *si, void *unused);
static void spawn_pc_thread();

pagemap_t *pagemap = NULL;
int thread_flag = 0;
extern int chunk_size;
extern thread_t thread;

void *copy_read(log_t *log, char *var_name,int process_id){
	void *buffer=NULL;
    void *data_addr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	data_addr = get_data_addr(cbptr,checkpoint);
    buffer = malloc(checkpoint->data_size);
	nvmmemcpy_read(buffer,data_addr,checkpoint->data_size);
	return buffer;
}

void *fault_read(log_t *log, char *var_name, int process_id){
	void *ddataptr=NULL;
    void *nvdataptr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	long page_size = sysconf(_SC_PAGESIZE);
	long page_aligned_size = ((checkpoint->data_size+page_size-1)& ~(page_size-1));
	if(isDebugEnabled()){
		printf("fault_read: actual size and page aligned size %ld : %ld \n", checkpoint->data_size, page_aligned_size);
	}
	nvdataptr = get_data_addr(cbptr,checkpoint);
    int s = posix_memalign(&ddataptr,page_size, page_aligned_size);
	if (s != 0){
        handle_error("posix memalign");
	}
	install_sighandler(bchandler);
	enable_protection(ddataptr, page_aligned_size);
	put_pagemap(&pagemap,ddataptr, nvdataptr, checkpoint->data_size, page_aligned_size);
	return ddataptr;
}



static void bchandler(int sig, siginfo_t *si, void *unused){
	if(isDebugEnabled()){
		printf("bchandler:page fault handler begin\n");
	}	
	pagemap_t *pagenode = get_pagemap(&pagemap, si->si_addr);
    disable_protection(pagenode,si->si_addr);
	if(isDebugEnabled()){
		printf("bchandler:page fault handler end\n");
	}	
}



void *pc_read(log_t *log, char *var_name, int process_id){
	void *ddataptr=NULL;
    void *nvdataptr = NULL;

	if(!thread_flag){
		spawn_pc_thread();
		thread_flag = 1;
	}

	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	long page_size = sysconf(_SC_PAGESIZE);
	long page_aligned_size = ((checkpoint->data_size+page_size-1)& ~(page_size-1));
	if(isDebugEnabled()){
		printf("fault_read: actual size and page aligned size %ld : %ld \n", checkpoint->data_size, page_aligned_size);
	}
	nvdataptr = get_data_addr(cbptr,checkpoint);
    int s = posix_memalign(&ddataptr,page_size, page_aligned_size);
	if (s != 0){
        handle_error("posix memalign");
	}
	install_sighandler(pchandler);
	enable_protection(ddataptr, page_aligned_size);
	put_pagemap(&pagemap,ddataptr, nvdataptr, checkpoint->data_size, page_aligned_size);

	return ddataptr;
}



static void pchandler(int sig, siginfo_t *si, void *unused){
	if(isDebugEnabled()){
		printf("bchandler:page fault handler begin\n");
	}	
	pagemap_t *pagenode = get_pagemap(&pagemap, si->si_addr);
    disable_protection(pagenode,si->si_addr);
	if(!thread.flag){
		if(isDebugEnabled()){
			printf("pchandler: raising the SIGUSR1 signal.\n");
		}
		kill(getpid(),SIGUSR1);
		thread.flag = 1;
	}
	if(isDebugEnabled()){
		printf("bchandler:page fault handler end\n");
	}	
}


static void *pc_func(void *arg){
	if(isDebugEnabled()){
		printf("pc_func: pre-fetch thread started\n");
	}
	tcontext_t *tcontext = (tcontext_t *)arg;
	sigset_t sigs_to_catch;	
	int caught;	
	sigemptyset(&sigs_to_catch);	
	sigaddset(&sigs_to_catch, SIGUSR1);
	if(isDebugEnabled()){
		printf("pc_func: waiting for SIGUSR1 signal\n");
	}
	sigwait(&sigs_to_catch, &caught);// wait till the memory access occur	
	if(isDebugEnabled()){
		printf("pc_func: starting the pre-fetch thread\n");
	}
	while(1){
		printf("fetching new memory.\n");
		sleep(1);
	}
	return (void *)1;
}

/*block signal USER1 from all threads and spawn a thread that handles the 
  signal. This is the pre-copy thread. */
static void spawn_pc_thread(){
	int s;
	sigset_t sigs_to_block;
	sigemptyset(&sigs_to_block);	
	sigaddset(&sigs_to_block, SIGUSR1);	
	pthread_sigmask(SIG_BLOCK, &sigs_to_block, NULL);
	
	if(isDebugEnabled()){
		printf("spawning the pre-fetch thread. \n");
	}
	thread.flag = 0;
	tcontext_t *tcontext  = malloc(sizeof(tcontext_t));
	s = pthread_create(&thread.pthreadid,NULL, pc_func, tcontext);	
	if(s!=0){
		handle_error("pthread creation failed\n");
	}
	return;
}

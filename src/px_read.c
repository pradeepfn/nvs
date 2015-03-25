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


/*
* naive copy read. copies the data from NVRAM to DRAM upon call
*/
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


/*
* fault read copies the data during a page fault
*/
void *fault_read(log_t *log, char *var_name, int process_id,void (*sighandler)(int,siginfo_t *,void *)){
	void *ddata_ptr=NULL;
    void *nvdata_ptr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	long page_size = sysconf(_SC_PAGESIZE);
    /*
		for a page size of 4, and a data size of 5, then the aligned data size should be 8.
		0101 + 0011 = 1000,
		and then we cut out the lower two bits.  
	*/
	long page_aligned_size = ((checkpoint->data_size+page_size-1)& ~(page_size-1));
	if(isDebugEnabled()){
		printf("fault_read: actual size and page aligned size %ld : %ld \n", checkpoint->data_size, page_aligned_size);
	}
	nvdata_ptr = get_data_addr(cbptr,checkpoint);
    int s = posix_memalign(&ddata_ptr,page_size, page_aligned_size);
	if (s != 0){
        handle_error("posix memalign");
	}
	install_sighandler(sighandler);
	enable_protection(ddata_ptr, page_aligned_size);
	put_pagemap(&pagemap,ddata_ptr, nvdata_ptr, checkpoint->data_size, page_aligned_size);
	return ddata_ptr;
}

/*
* this hander copies the faulted chunk (entire variable) 
*/
static void bchandler(int sig, siginfo_t *si, void *unused){
	long size_of_variable;
	if(isDebugEnabled()){
		printf("bchandler:page fault handler begin\n");
	}	
	//the addres returned from the si->si_addr is a random adress
    // within the allocated range
	pagemap_t *pagenode = get_pagemap(&pagemap, si->si_addr);
	pagenode->copied = 1;
    size_of_variable = disable_protection(pagenode->pageptr,pagenode->paligned_size);
	assert(size_of_variable != pagenode->size);
	nvmmemcpy_read(pagenode->pageptr,pagenode->nvpageptr,pagenode->size);
	if(isDebugEnabled()){
		printf("bchandler:page fault handler end\n");
	}	
}

void *chunk_read(log_t *log, char *var_name, int process_id){
	return fault_read(log,var_name,process_id,bchandler);
}

/*
* pre copy read function
*/
void *pc_read(log_t *log, char *var_name, int process_id){
	if(!thread_flag){
		spawn_pc_thread();
		thread_flag = 1;
	}
	return fault_read(log,var_name,process_id,pchandler);
}

/*
* copy the faulted memory portion and raise and notify the pre-fetch
* thread, if not notified already.
*/
static void pchandler(int sig, siginfo_t *si, void *unused){
	bchandler(sig,si,unused);
	if(!thread.flag){
		if(isDebugEnabled()){
			printf("pchandler: raising the SIGUSR1 signal.\n");
		}
		kill(getpid(),SIGUSR1);
	}
}


static void *pc_func(void *arg){
	//tcontext_t *tcontext = (tcontext_t *)arg;
	pthread_detach(pthread_self()); // self cleanup after terminate

	if(isDebugEnabled()){
		printf("pc_func: pre-fetch thread started\n");
	}
	// newly spawned thread wait on the SIGUSR1 upon spawn
	int caught;	
	sigset_t sigs_to_catch;	
	sigemptyset(&sigs_to_catch);	
	sigaddset(&sigs_to_catch, SIGUSR1);
	if(isDebugEnabled()){
		printf("pc_func: waiting for SIGUSR1 signal\n");
	}
	// wait till the memory access occure
	sigwait(&sigs_to_catch, &caught);
	if(isDebugEnabled()){
		printf("pc_func: starting the pre-fetch thread\n");
	}
	// once signal received for the first time, we mark the thread as spwaned
	thread.flag = 1;
	copy_chunks(&pagemap);
	return (void *)1;
}

/*block signal USER1 from all threads and spawn a thread that handles the 
  signal. This is the pre-copy thread. */
static void spawn_pc_thread(){
	int s;
	//block the SIGUSR1 signal in main thread.
	sigset_t sigs_to_block;
	sigemptyset(&sigs_to_block);	
	sigaddset(&sigs_to_block, SIGUSR1);	
	pthread_sigmask(SIG_BLOCK, &sigs_to_block, NULL);
	
	if(isDebugEnabled()){
		printf("spawning the pre-fetch thread. \n");
	}
	thread.flag = 0;
	tcontext_t *tcontext  = malloc(sizeof(tcontext_t));
	//execute the pre-fetch thread
	s = pthread_create(&thread.pthreadid,NULL, pc_func, tcontext);	
	if(s!=0){
		handle_error("pthread creation failed\n");
	}
	return;
}

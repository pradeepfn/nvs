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
#include "px_remote.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void bchandler(int sig, siginfo_t *si, void *unused);
static void pchandler(int sig, siginfo_t *si, void *unused);
static void rbchandler(int sig, siginfo_t *si, void *unused);
static void rpchandler(int sig, siginfo_t *si, void *unused);
void remote_fault_read(log_t *log, char *var_name, int process_id,void (*sighandler)(int,siginfo_t *,void *),entry_t *entry);
static void spawn_pc_thread();
static void *pc_func(void *arg);
static void *rpc_func(void *arg);

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


void *page_aligned_copy_read(log_t *log, char *var_name,int process_id){
	void *ddata_ptr = NULL;
	void *nvdata_ptr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	nvdata_ptr = get_data_addr(cbptr,checkpoint);
	//allocating page aligned memory
	long page_size = sysconf(_SC_PAGESIZE);
	long page_aligned_size = ((checkpoint->data_size+page_size-1)& ~(page_size-1));
	if(isDebugEnabled()){
		printf("fault_read: actual size and page aligned size %ld : %ld \n", checkpoint->data_size, page_aligned_size);
	}
    int s = posix_memalign(&ddata_ptr,page_size, page_aligned_size);
	if (s != 0){
        handle_error("posix memalign");
	}
	nvmmemcpy_read(ddata_ptr,nvdata_ptr,checkpoint->data_size);
	return ddata_ptr;
}

void remote_copy_read(log_t *log, char *var_name,int process_id, entry_t *entry){
    void *data_addr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
    }
	data_addr = get_data_addr(cbptr,checkpoint);

    entry->ptr = malloc(checkpoint->data_size);
	entry->local_ptr = remote_alloc(&entry->rmt_ptr,checkpoint->data_size);
	//copy the data to local armci memory
	memcpy(entry->local_ptr,data_addr,checkpoint->data_size);
	//get the remote data to localy allocated memory
	remote_read(entry->ptr,entry->rmt_ptr, checkpoint->data_size);
}


void remote_chunk_read(log_t *log, char *var_name,int process_id,entry_t *entry){
	remote_fault_read(log,var_name,process_id,rbchandler,entry);
}


void remote_pc_read(log_t *log, char *var_name,int process_id,entry_t *entry){
	remote_fault_read(log,var_name,process_id,rpchandler,entry);
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
	put_pagemap(&pagemap,ddata_ptr, nvdata_ptr, checkpoint->data_size, page_aligned_size,NULL);
	return ddata_ptr;
}


void remote_fault_read(log_t *log, char *var_name, int process_id,void (*sighandler)(int,siginfo_t *,void *),entry_t *entry){
    void *nvdata_ptr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
    }
	long page_size = sysconf(_SC_PAGESIZE);
	long page_aligned_size = ((checkpoint->data_size+page_size-1)& ~(page_size-1));
	if(isDebugEnabled()){
		printf("remote fault_read: actual size and page aligned size %ld : %ld \n", checkpoint->data_size, page_aligned_size);
	}
	// create local DRAM portion
	nvdata_ptr = get_data_addr(cbptr,checkpoint);
    int s = posix_memalign(&entry->ptr,page_size, page_aligned_size);
	if (s != 0){
        handle_error("posix memalign");
	}
	// create group memory portions and copy the checkpoint data to them
	entry->local_ptr = remote_alloc(&entry->rmt_ptr,checkpoint->data_size);
	//TODO: get rid of this copy
	memcpy(entry->local_ptr,nvdata_ptr,checkpoint->data_size);

	install_sighandler(sighandler);
	enable_protection(entry->ptr, page_aligned_size);
	put_pagemap(&pagemap,entry->ptr, NULL, checkpoint->data_size, page_aligned_size,entry->rmt_ptr);
}


/*
* this hander copies the faulted chunk (entire variable) 
*/
static void bchandler(int sig, siginfo_t *si, void *unused){
	long size_of_variable;

	if(isDebugEnabled()){
		printf("bchandler:page fault handler begin\n");
	}	

	if(si != NULL && si->si_addr !=  NULL){
		//the addres returned from the si->si_addr is a random adress
		// within the allocated range
		pagemap_t *pagenode = get_pagemap(&pagemap, si->si_addr);
		pagenode->copied = 1;
		size_of_variable = disable_protection(pagenode->pageptr,pagenode->paligned_size);
		if(isDebugEnabled()){
			printf("size of variable : %ld, pagenode->size : %ld  pagenode->alignedsize : %ld\n",size_of_variable,pagenode->size,pagenode->paligned_size);
		}
		nvmmemcpy_read(pagenode->pageptr,pagenode->nvpageptr,pagenode->size);
		if(isDebugEnabled()){
			printf("bchandler:page fault handler end\n");
		}
	}else{ 
		// this is a Original SIGSEGV segfault signal it seems.
		call_oldhandler(sig);
	}	
}


static void rbchandler(int sig, siginfo_t *si, void *unused){
	long size_of_variable;

	if(isDebugEnabled()){
		printf("rbchandler:page fault handler begin\n");
	}	

	if(si != NULL && si->si_addr !=  NULL){
		//the addres returned from the si->si_addr is a random adress
		// within the allocated range
		pagemap_t *pagenode = get_pagemap(&pagemap, si->si_addr);
		pagenode->copied = 1;
		size_of_variable = disable_protection(pagenode->pageptr,pagenode->paligned_size);
		if(isDebugEnabled()){
			printf("size of variable : %ld, pagenode->size : %ld  pagenode->alignedsize : %ld\n",size_of_variable,pagenode->size,pagenode->paligned_size);
		}
		//copying from the remote node
		remote_read(pagenode->pageptr,pagenode->remote_ptr, pagenode->size);

		if(isDebugEnabled()){
			printf("rbchandler:page fault handler end\n");
		}
	}else{ 
		// this is a Original SIGSEGV segfault signal it seems.
		call_oldhandler(sig);
	}

}

static void rpchandler(int sig, siginfo_t *si, void *unused){
	rbchandler(sig,si,unused);
	if(!thread_flag){
		spawn_pc_thread(rpc_func);
		thread_flag = 1;
	}
}

void *chunk_read(log_t *log, char *var_name, int process_id){
	return fault_read(log,var_name,process_id,bchandler);
}

/*
* pre copy read function
*/
void *pc_read(log_t *log, char *var_name, int process_id){
	return fault_read(log,var_name,process_id,pchandler);
}

/*
* copy the faulted memory portion and raise and notify the pre-fetch
* thread, if not notified already.
*/
static void pchandler(int sig, siginfo_t *si, void *unused){
	bchandler(sig,si,unused);
	if(!thread_flag){
		spawn_pc_thread(pc_func);
		thread_flag = 1;
	}
}




static void *rpc_func(void *arg){
	//tcontext_t *tcontext = (tcontext_t *)arg;
	pthread_detach(pthread_self()); // self cleanup after terminate
	if(isDebugEnabled()){
		printf("rpc_func: pre-fetch thread started\n");
	}
	copy_remote_chunks(&pagemap);
	return (void *)1;
}


static void *pc_func(void *arg){
	//tcontext_t *tcontext = (tcontext_t *)arg;
	pthread_detach(pthread_self()); // self cleanup after terminate
	if(isDebugEnabled()){
		printf("pc_func: pre-fetch thread started\n");
	}
	copy_chunks(&pagemap);
	return (void *)1;
}

/*block signal USER1 from all threads and spawn a thread that handles the 
  signal. This is the pre-copy thread. */
static void spawn_pc_thread(void *(*func)(void*)){
	int s;
	if(isDebugEnabled()){
		printf("spawning the pre-fetch thread. \n");
	}
	tcontext_t *tcontext  = malloc(sizeof(tcontext_t));
	//execute the pre-fetch thread
	s = pthread_create(&thread.pthreadid,NULL, func, tcontext);	
	if(s!=0){
		handle_error("pthread creation failed\n");
	}
	return;
}

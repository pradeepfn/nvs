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

pagemap_t *get_pagemap(void *pageptr);
void put_pagemap(void *pageptr, void *nvpageptr, offset_t size);
int disable_protection(pagemap_t *pagenode);
int enable_protection(void *ptr, size_t size);
void install_bchandler();
static void bchandler(int sig, siginfo_t *si, void *unused);

pagemap_t *pagemap = NULL;

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
	install_bchandler();
	enable_protection(ddataptr, page_aligned_size);
	put_pagemap(ddataptr, nvdataptr, checkpoint->data_size);
	return ddataptr;
}



static void bchandler(int sig, siginfo_t *si, void *unused){
	if(isDebugEnabled()){
		printf("bchandler:page fault handler begin\n");
	}	
	pagemap_t *pagenode = get_pagemap(si->si_addr);
    disable_protection(pagenode);
	nvmmemcpy_read(pagenode->pageptr, pagenode->nvpageptr, pagenode->size);
	if(isDebugEnabled()){
		printf("bchandler:page fault handler end\n");
	}	
}

void install_bchandler(){
    struct sigaction sa; 
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = bchandler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1){ 
        handle_error("sigaction");
	}
}

int enable_protection(void *ptr, size_t size) {
	if (mprotect(ptr, size,PROT_NONE) == -1){
        handle_error("mprotect");
	}
    return 0;
}

int disable_protection(pagemap_t *pagenode){
	if (mprotect(pagenode->pageptr, pagenode->size,PROT_READ|PROT_WRITE) == -1){
        handle_error("mprotect");
	}
    return 0;
}

void put_pagemap(void *pageptr, void *nvpageptr, offset_t size){
	pagemap_t *s;
	HASH_FIND_INT(pagemap, &pageptr, s);
    if (s==NULL) {
		s = (pagemap_t *)malloc(sizeof(pagemap_t));
		s->pageptr = pageptr;
		s->nvpageptr = nvpageptr;
		s->size = size;
		HASH_ADD_INT( pagemap, pageptr, s );
	}
}

pagemap_t *get_pagemap(void *pageptr){
	pagemap_t *s, *tmp;
	HASH_ITER(hh, pagemap, s, tmp) {
		if(s->pageptr == pageptr){ // TODO: range check
			return s;
		}
	}
	handle_error("address not found in pagemap");
}

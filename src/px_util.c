#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>

#include "px_util.h"

//#define NVRAM_BW  450
#define NVRAM_W_BW  600
#define NVRAM_R_BW  2*NVRAM_W_BW
#define MICROSEC 1000000

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

extern int chunk_size;

unsigned long calc_delay_ns(size_t datasize,int bandwidth){
        unsigned long delay;
        double data_MB, sec;

        data_MB = (double)((double)datasize/(double)pow(10,6));
        sec =(double)((double)data_MB/(double)bandwidth);
        delay = sec * pow(10,9);
        return delay;
}

int __nsleep(const struct timespec *req, struct timespec *rem)
{
    struct timespec temp_rem;
    if(nanosleep(req,rem)==-1)
        __nsleep(rem,&temp_rem);
    else
        return 1;
}

int msleep(unsigned long nanosec)
{
    struct timespec req={0},rem={0};
    time_t sec=(int)(nanosec/1000000000);
    req.tv_sec=sec;
    req.tv_nsec=nanosec%1000000000;
    __nsleep(&req,&rem);
    return 1;
}


int nvmmemcpy_read(void *dest, void *src, size_t len) {
    unsigned long lat_ns;
    lat_ns = calc_delay_ns(len,NVRAM_R_BW);
    msleep(lat_ns);
    memcpy(dest,src,len);
    return 0;
}

int nvmmemcpy_write(void *dest, void *src, size_t len) {
    unsigned long lat_ns;
    lat_ns = calc_delay_ns(len,NVRAM_W_BW);
    msleep(lat_ns);
    memcpy(dest,src,len);
    return 0;
}

void *get_data_addr(void *base_addr, checkpoint_t *chkptr){
    char *temp = ((char *)base_addr) + chkptr->offset + sizeof(checkpoint_t);
    return (void *)temp;
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) 
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / MICROSEC + 1;
    y->tv_usec -= MICROSEC * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > MICROSEC) {
    int nsec = (x->tv_usec - y->tv_usec) / MICROSEC;
    y->tv_usec += MICROSEC * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}


void install_sighandler(void (*sighandler)(int,siginfo_t *,void *)){
    struct sigaction sa; 
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sighandler;
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
/*Disable protection in chunk wise. Chunks are multiples of pages */
int disable_protection(pagemap_t *pagenode, void *fault_addr){
	int length;
	offset_t disposition = (fault_addr - pagenode->pageptr);
	long chunk_index = disposition & ~(chunk_size-1);
	long chunk_index_of_last_page = pagenode->paligned_size & ~(chunk_size-1);
	void *chunk_start_addr = (void *)(((char *)pagenode->pageptr) + (chunk_index*chunk_size));
	if(chunk_index == chunk_index_of_last_page){
		length = pagenode->paligned_size % chunk_size;
	}else if(chunk_index < chunk_index_of_last_page){
		length = chunk_size;
	}else{
		printf("disable_protection: chunk index should fall within memory block\n");
		assert(0);
	}
	if (mprotect(chunk_start_addr,length,PROT_READ|PROT_WRITE) == -1){
        handle_error("mprotect");
	}
	nvmmemcpy_read(pagenode->pageptr, pagenode->nvpageptr, length);
    return 0;
}

void put_pagemap(pagemap_t **pagemapptr ,void *pageptr, void *nvpageptr, offset_t size, offset_t asize){
	pagemap_t *s;
	HASH_FIND_INT(*pagemapptr, &pageptr, s);
    if (s==NULL) {
		s = (pagemap_t *)malloc(sizeof(pagemap_t));
		s->pageptr = pageptr;
		s->nvpageptr = nvpageptr;
		s->size = size;
		s->size = asize;
		HASH_ADD_INT( *pagemapptr, pageptr, s );
	}
}

pagemap_t *get_pagemap(pagemap_t **pagemapptr, void *pageptr){
	pagemap_t *s, *tmp;
	HASH_ITER(hh, *pagemapptr, s, tmp) {
		if(s->pageptr == pageptr){ // TODO: range check
			return s;
		}
	}
	handle_error("address not found in pagemap");
}

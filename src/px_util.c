#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "px_util.h"
#include "px_remote.h"
#include "px_debug.h"
#include "px_constants.h"

// read bandwidth to constant maching
// 2048Mb/s -> 600
// 1024Mb/s -> 300 
// 512Mb/s -> 150
// 256Mb/s -> 75
// 128Mb/s -> 38
// 64Mb/s ->19
// -1 relates to DRAM -> no delay


#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

extern int chunk_size;
extern int nvram_wbw;
extern int lib_process_id;
struct sigaction old_sa; 

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
    if(nanosleep(req,rem)==-1){
        __nsleep(rem,&temp_rem);
    }
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
	if(nvram_wbw > 0){
		unsigned long lat_ns;
		//read bandwidth is taken as the two times, write bandwidth
		lat_ns = calc_delay_ns(len,nvram_wbw*2);
		msleep(lat_ns);
	}
    memcpy(dest,src,len);
    return 0;
}

int nvmmemcpy_write(void *dest, void *src, size_t len) {
	if(nvram_wbw > 0){
		unsigned long lat_ns;
		lat_ns = calc_delay_ns(len,nvram_wbw);
		msleep(lat_ns);
	}
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
    if (sigaction(SIGSEGV, &sa, &old_sa) == -1){ 
        handle_error("sigaction");
	}
}


void install_old_handler(){
    struct sigaction temp;
    debug("old signal handler installed");
    if (sigaction(SIGSEGV, &old_sa, &temp) == -1){
        handle_error("sigaction");
    }
}

void call_oldhandler(int signo){
    debug("old signal handler called");
    (*old_sa.sa_handler)(signo);
}

void enable_protection(void *ptr, size_t size) {
	if (mprotect(ptr, size,PROT_NONE) == -1){
        handle_error("mprotect");
	}
}

/*
* Disable the protection of the whole aligned memory block related to each variable access
* return : size of memory
*/
long disable_protection(void *page_start_addr,size_t aligned_size){
	if (mprotect(page_start_addr,aligned_size,PROT_WRITE) == -1){
        handle_error("mprotect");
	}
    return aligned_size;
}

/*
* put an element to pagemap.wrapper method
*/
void pagemap_put(pagemap_t **pagemapptr, char *varname, void *pageptr, void *nvpageptr, offset_t size, offset_t asize,
                 void **memory_grid){
	pagemap_t *s;
	HASH_FIND_PTR(*pagemapptr, &pageptr, s);
    if (s==NULL) {
		s = (pagemap_t *)malloc(sizeof(pagemap_t));
		s->pageptr = pageptr;
		s->nvpageptr = nvpageptr;
		s->size = size;
		s->paligned_size = asize;
		s->copied = 0;
		s->remote_ptr = memory_grid;
        s->started_tracking = 0;
        s->start_timestamp = (struct timeval) {0,0};
        s->end_timestamp = (struct timeval) {0,0};

		memcpy(s->varname,varname,sizeof(char)*20);
		HASH_ADD_INT( *pagemapptr, pageptr, s );
	}
}

/*
* get an element to pagemap.wrapper method
*/
pagemap_t *pagemap_get(pagemap_t **pagemapptr, void *pageptr){
	pagemap_t *s, *tmp;
	long offset = 0;
	if(isDebugEnabled()){
		printf("memory address trying to access %p\n",pageptr);
	}
	HASH_ITER(hh, *pagemapptr, s, tmp) {
		offset = pageptr - s->pageptr;
		if(offset >=0 && offset <= s->size){ // the adress belong to this chunk.
			/*if(isDebugEnabled()){
				printf("starting address of the matching chunk %p\n",s->pageptr);
			}*/
			return s;
		}
	}
	handle_error("address not found in pagemap");
}

/*
* this utility method copies all the not copied data from NVRAM to DRAM
* the pre-copy thread make use of this method
*/
void copy_chunks(pagemap_t **page_map_ptr){
	pagemap_t *s, *tmp;
	//1. iterate and copy all the not copied chunks.
	//2. disable the protection
	//3. copy them to DRAM location
	HASH_ITER(hh, *page_map_ptr, s, tmp) {
		if(!s->copied){
			if(isDebugEnabled()){
				printf("pre fetching variable : %s \n",s->varname);
			}
			disable_protection(s->pageptr,s->paligned_size);
			
			nvmmemcpy_read(s->pageptr,s->nvpageptr,s->size);	
			s->copied = 1;
		}	
	}
}

void copy_remote_chunks(pagemap_t **page_map_ptr){
	pagemap_t *s, *tmp;
	//1. iterate and copy all the not copied chunks.
	//2. disable the protection
	//3. copy them to DRAM location
	HASH_ITER(hh, *page_map_ptr, s, tmp) {
		if(!s->copied){
			if(isDebugEnabled()){
				printf("[%d] remote pre-fetch name : %s , size : %ld alignedsize : %ld \n",lib_process_id, s->varname,s->size, s->paligned_size);
			}
			disable_protection(s->pageptr,s->paligned_size);
			remote_read(s->pageptr,s->remote_ptr,s->size);	
			s->copied = 1;
		}	
	}
}

extern int n_processes;
extern int buddy_offset;

int get_mypeer(int myrank){
    int mypeer;

    if(myrank % 2 == 0) { //FIXME: This logic fails when offset is a even number
        mypeer = (myrank + buddy_offset) % n_processes;
        return mypeer;
    }else {
        if(myrank >= buddy_offset){
            mypeer = myrank - buddy_offset;
            return mypeer;
        } else{
            mypeer = n_processes + myrank - buddy_offset;
            return mypeer;
        }
    }
}

extern int split_ratio;
extern int cr_type;

void split_checkpoint_data(listhead_t *head) {
    entry_t *np;
    int i;
    long page_aligned_size;
    long page_size = sysconf(_SC_PAGESIZE);

    for(np = head->lh_first,i=0; np != NULL; np = np->entries.le_next,i++){
        if(i<split_ratio){
            np->type = DRAM_CHECKPOINT;
            page_aligned_size = ((np->size+page_size-1)& ~(page_size-1));
            if(lib_process_id == 0) {
                log_info("[%d] variable : %s  chosen for DRAM checkpoint\n",
                         lib_process_id,np->var_name);
            }
            if(cr_type == ONLINE_CR){
                debug("[%d] allocated remote DRAM pointers for variable %s",
                      lib_process_id ,np->var_name);
                np->local_ptr = remote_alloc(&np->rmt_ptr,page_aligned_size);
            }
        }
    }
}

/*
 * input char array is size of 20
 * we null terminate it at the first space
 */
char* null_terminate(char *c_string){
    int i;
    for(i=0;i<19;i++){
        if(c_string[i] == ' '){
            c_string[i] = '\0';
            return c_string;
        }
    }
    c_string[i] = '\0';
    return c_string;
}

/*
 * find whether we have dram destined data. we partition the variable space.
 */
int is_dlog_checkpoing_data_present(listhead_t *head){
    entry_t *np;
    int i;

    for(np = head->lh_first,i=0; np != NULL; np = np->entries.le_next,i++){
        if(np->type == DRAM_CHECKPOINT){
            return 1;
        }
    }
    return 0;
}
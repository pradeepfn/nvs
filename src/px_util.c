#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "px_util.h"
#include "px_debug.h"

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


int nvmmemcpy_read(void *dest, void *src, size_t len, int rbw) {
	if(rbw > 0){
		unsigned long lat_ns;
		//read bandwidth is taken as the two times, write bandwidth
		lat_ns = calc_delay_ns(len,rbw);
		msleep(lat_ns);
	}
    memcpy(dest,src,len);
    return 0;
}

int nvmmemcpy_write(void *dest, void *src, size_t len, int wbw) {
	if(wbw > 0){
		unsigned long lat_ns;
		lat_ns = calc_delay_ns(len,wbw);
		msleep(lat_ns);
	}
    memcpy(dest,src,len);
    return 0;
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

void enable_write_protection(void *ptr, size_t size) {
    if (mprotect(ptr, size,PROT_READ) == -1){
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
int is_dlog_checkpoing_data_present(var_t *list){
    var_t *s;
    int i;

    for(s = list,i=0; s != NULL; s = s->hh.next,i++){
        if(s->type == DRAM_CHECKPOINT){
            return 1;
        }
    }
    return 0;
}
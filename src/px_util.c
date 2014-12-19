#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "px_util.h"

//#define NVRAM_BW  450
#define NVRAM_W_BW  600
#define NVRAM_R_BW  2*NVRAM_W_BW
#define MICROSEC 1000000

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


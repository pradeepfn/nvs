
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "px_sampler.h"
#include "px_debug.h"

// we are taking the minimal free memory available in this node
long long min_free_mem = -1;
pthread_t thread;
int stopIssued = 0; // less than cache line size. Atomic update
pthread_mutex_t stopMutex; // instead of memory barrier we are using a mutex here


void* thread_function(void *tdata);

struct pthread_data{
    int dummy;
};


int is_stop_issued(void) {
    int ret = 0;
    pthread_mutex_lock(&stopMutex);
    ret = stopIssued;
    pthread_mutex_unlock(&stopMutex);
    return ret;
}

void set_stop_issued(void) {
    pthread_mutex_lock(&stopMutex);
    stopIssued = 1;
    pthread_mutex_unlock(&stopMutex);
}


void start_memory_sampling_thread(){
    struct pthread_data targs;
    debug("creating memory sampling thread\n");
    int status = pthread_create(&thread,NULL,thread_function,(void *)&targs);
    if(status){
        log_err("error creating monitor thread\n");
        exit(status);
    }
    return;
}


void stop_memory_sampling_thread(){
    set_stop_issued();
    debug("issuing command to stop memory sampling thread.");
}

long long get_free_memory(){
    return min_free_mem;
}

long long get_free_ram(){
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    long long memory_available = memInfo.freeram;
    memory_available *= memInfo.mem_unit;

    return memory_available;

}

void* thread_function(void *tdata){
    //struct pthread_data *data = (struct pthread_data *) tdata;
    while(1) {
        long long free_mem = get_free_ram();
        if (min_free_mem == -1 || free_mem < min_free_mem) {
            min_free_mem = free_mem;
        }
        if(is_stop_issued()){
            break;
        }
        usleep(20); // TODO : use nanosleep
    }
    debug("memory sampling thread exiting..\n");
    pthread_exit(NULL);
}


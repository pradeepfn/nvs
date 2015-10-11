//
// Created by pradeep on 10/9/15.
//

#include <pthread.h>
#include "px_earlyreadwrite.h"
#include "px_debug.h"



pthread_t early_copy_thread;
int cancel_thread=0;

void * data_write(void *stat_map);

/*
 * Writes NVM bound checkpoint data, to NVRAM. It gets the tracked data as input from the sampler
 * and start writing data as early as possible.
 *
 * Important : calling to this method spawns a early copy thread.
 *
 * input:
 * stats on variable access pattern during first checkpoint cycle
 */

int start_write_thread(pagemap_t *stat_map){

    int thread_status = pthread_create(&early_copy_thread,NULL,data_write,(void *)NULL);
    if(thread_status){
        log_err("erro while creating early copy thread\n");
        exit(1);
    }
    return 0;
}

void * early_read(){
    return NULL;
}

/*
 * 1. sleep till our estimated first variable write time.
 * 2. write the variable
 * 3. sleep till next variable
 * 4. If the application
 */
void* data_write(void *stat_map){
    return NULL;
}
/*
 * setting thread cancel flag.
 * and wait the thread to finish
 */
void stop_write_thread(){
    void *ret;

    cancel_thread = 1;
    pthread_join(early_copy_thread,&ret);
    if((long)ret != 0){
        log_err("error shutting down early copy thread\n");
    }
    return;
}
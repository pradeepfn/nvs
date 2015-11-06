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
#include "px_constants.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


pagemap_t *pagemap = NULL;
extern int chunk_size;
extern thread_t thread;
extern int lib_process_id;


void* local_dram_read(dlog_t *dlog, char *var_name,int process_id, int version);

/*
* naive copy read. copies the data from NVRAM to DRAM upon call
*/
void *copy_read(log_t *log, char *var_name,int process_id, long version){
	void *buffer=NULL;
    void *data_addr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id,version);
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
 * If you are a surviving process, then
 *
 * 1. scan the nvlog and identify  whats avaible in DRAM
 * 2. read from DRAM if possible otherwise from nvram
 *
 * if you are the failed process,then
 *
 * 1. init process
 * 2. scan nvlog and identify what available in DRAM
 * 3. read from nvlog while delegating remote DRAM reading to another thread
 *
 * if you are the buddy process of failed process, then
 *
 * 1. do the same steps as surviving process
 * 2. serve the failed process request. (that data is still in DRAM)
 *
 */
void *online_copy(log_t *log,dlog_t *dlog, char *var_name, int process_id , int failed_process){
    /*void *buffer;
    long recover_version = log->current->head->current_version;
    if(process_id == failed_process){ // failed process
        // read from NVLOG. if not found, then
        // TODO: not supported

    }else{

        debug("[%d] using the checkpoing version : %ld",lib_process_id, recover_version);
        //first check dlog
        buffer = local_dram_read(dlog,var_name,process_id,recover_version);

        // if not found, check nvlog
        if(buffer == NULL) {
            return copy_read(log, var_name, process_id, recover_version);
        }else{
            return buffer;
        }
    }*/

    return NULL;
}




void* local_dram_read(dlog_t *dlog, char *var_name,int process_id, int version){
    if(dlog->dlog_checkpoint_version != version){
        return  NULL;
    }
    dcheckpoint_map_entry_t *entry;
    HASH_FIND_STR(dlog->map[NVRAM_CHECKPOINT],var_name,entry);
    return entry; // NULL if not found
}


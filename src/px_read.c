#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "px_dlog.h"
#include "px_util.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


var_t *pagemap = NULL;
extern int chunk_size;
extern int lib_process_id;
extern int nvram_wbw;


void* local_dram_read(dlog_t *dlog, char *var_name,int process_id, int version);

/*
* naive copy read. copies the data from NVRAM to DRAM upon call
*/
var_t *copy_read(log_t *log, char *var_name,int process_id, long version){
	void *buffer=NULL;
    void *data_addr = NULL;
    int status;
    long page_aligned_size;
    var_t *s;

	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id,version);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
    int page_size = sysconf(_SC_PAGESIZE);
    page_aligned_size = ((checkpoint->data_size + page_size - 1) & ~(page_size - 1));
	data_addr = get_data_addr(cbptr,checkpoint);

    status = posix_memalign(&buffer, page_size, page_aligned_size);
    if (status != 0) {
        return NULL;
    }

    s = (var_t *)malloc(sizeof(var_t));
    s->ptr = buffer;
    s->size = checkpoint->data_size;
    s->process_id = process_id;
    s->paligned_size = page_aligned_size;
    memcpy(s->varname,var_name,sizeof(char)*20);
	nvmmemcpy_read(s->ptr,data_addr,checkpoint->data_size,nvram_wbw*2);

	return s;
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


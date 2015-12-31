#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

#include "px_log.h"
#include "px_debug.h"
#include "px_constants.h"
#include "px_util.h"

#define FILE_PATH_ONE "/mmap.file.meta" // stores the ring buffer
#define FILE_PATH_TWO "/mmap.file.data" // stores linear metadata


int is_chkpoint_present(log_t *log);
static void init_mmap_files(log_t *log);

checkpoint_t* ringb_element(log_t *log, ulong index);
void* log_ptr(log_t *log,ulong offset);

int log_init(log_t *log , int proc_id){
    ccontext_t *config_context = log->runtime_context->config_context;
	if(isDebugEnabled()){
		printf("initializing the structures... %d \n", proc_id);
	}
    pthread_mutex_init(&(log->plock),NULL);
	log->data_log.log_size = config_context->log_size;
    snprintf(log->ring_buffer.file_name, sizeof(log->ring_buffer.file_name), "%s%s%d",
            config_context->pfile_location,FILE_PATH_ONE, proc_id);
    snprintf(log->data_log.file_name, sizeof(log->data_log.file_name),"%s%s%d",
             config_context->pfile_location,FILE_PATH_TWO, proc_id);
    init_mmap_files(log);
    return 0;
}

//update the current latest version of the log buffer and move the tail forward
int log_commitv(log_t *log,ulong version){

    if(version < 1){ //start GC after 1st version
        return 1;
    };

    pthread_mutex_lock(&log->plock);
    log->ring_buffer.head->current_version = version; // atomic commit
    //TODO: flush to fs
    //iterate starting from the tail and select a new tail
    checkpoint_t *rb_elem = ringb_element(log,log->ring_buffer.head->tail);
    while(rb_elem->version <= (version-1)){ 
        log->ring_buffer.head->tail = (log->ring_buffer.head->tail+1)%RING_BUFFER_SLOTS;
        log->ring_buffer.log_tail = rb_elem->end_offset+1;
        rb_elem = ringb_element(log,log->ring_buffer.head->tail);
    }
    pthread_mutex_unlock(&log->plock);
    return 1;
}


/* writing to the data to persistent storage*/

int log_write(log_t *log, var_t *variable, int process_id,long version){
    void *reserved_log_ptr,*preamble_log_ptr;
    ulong reserved_log_offset;
    ulong avail_size;

    debug("[%d] nvram checkpoint  varname : %s , process_id :  %d , version : %ld ", log->runtime_context->process_id,
          variable->varname, process_id, version);

    ulong checkpoint_size = variable->size + sizeof(struct preamble_t_);

    //get the log reservation
    pthread_mutex_lock(&(log->plock));
    //check if slots left in the ring buffer
    if(log_isfull(log)){
        log_err("no slots left in the ring buffer");
        pthread_mutex_unlock(&(log->plock));
        exit(1);
    }

    ulong dhead_offset = log->ring_buffer.log_head; // data head offset , point to the next free head element to write
    ulong dtail_offset = log->ring_buffer.log_tail; // data tail offset , point to tail of the data log. if equal to head, then empty
    ulong dlog_size = log->ring_buffer.head->log_size; // data log size


    if(dhead_offset >= dtail_offset){   //if head is infront,
        avail_size = dlog_size - dhead_offset;
        if(avail_size >= checkpoint_size){  // head to end of linear log
            reserved_log_offset = dhead_offset;
        }else if(dtail_offset >= checkpoint_size){   // start of the linear log to tail
            reserved_log_offset = 0;
        }else{
            log_err("not enough space left in the linear log");
            pthread_mutex_unlock(&(log->plock));
            log_info("[%d]linear log tail and head , %ld   %ld" ,
                     log->runtime_context->process_id,
                     log->ring_buffer.log_tail,
                     log->ring_buffer.log_head);
            log_info("[%d]ring buffer indexes , %ld  %ld",
                     log->runtime_context->process_id,
                     log->ring_buffer.head->tail,
                     log->ring_buffer.head->head);
            exit(1);
        }

    }else if(dtail_offset > dhead_offset){  // head is behind
        avail_size = dtail_offset - dhead_offset;
        if(avail_size >= checkpoint_size){
            reserved_log_offset = dhead_offset;
        }else{
            log_err("not enough space in the linear log");
            pthread_mutex_unlock(&(log->plock));
            log_info("[%d]linear log tail and head , %ld   %ld" ,
                     log->runtime_context->process_id,
                     log->ring_buffer.log_tail,
                     log->ring_buffer.log_head);
            log_info("[%d]ring buffer indexes , %ld  %ld",
                     log->runtime_context->process_id,
                     log->ring_buffer.head->tail,
                     log->ring_buffer.head->head);
            exit(1);
        }
    }
    // we are good to do the final reserve...
    ulong rb_index = log->ring_buffer.head->head;

    checkpoint_t *checkpoint_elem = ringb_element(log,rb_index);

    strncpy(checkpoint_elem->var_name,variable->varname,VAR_SIZE);
    checkpoint_elem->process_id = process_id;
    checkpoint_elem->version = version;
    checkpoint_elem->size = variable->size;
    checkpoint_elem->start_offset = reserved_log_offset;
    checkpoint_elem->end_offset = reserved_log_offset + checkpoint_size-1;

    log->ring_buffer.head->head = (log->ring_buffer.head->head + 1)%RING_BUFFER_SLOTS; // atomically commit slot reservation
    log->ring_buffer.log_head = checkpoint_elem->end_offset + 1; // this is a soft state
    pthread_mutex_unlock(&(log->plock));

#ifdef PX_DIGEST
    md5_digest(checkpoint_elem->hash,variable->ptr,variable->size);
    memcpy(variable->hash,checkpoint_elem->hash,MD5_LENGTH);
#endif

    //no lock operation
    reserved_log_ptr = log_ptr(log,reserved_log_offset);
    ulong preamble_offset = reserved_log_offset + variable->size;
    preamble_log_ptr = log_ptr(log,preamble_offset);

    // write to log on the granted log boundry and update the commit bit
    // valid_bit followed by data. we use the valid bit to atomically copy the checkpoint data.
    nvmmemcpy_write(reserved_log_ptr,variable->ptr,variable->size,log->runtime_context->config_context->nvram_wbw);
    preamble_t preamble;
    preamble.value = MAGIC_VALUE;
    memcpy(preamble_log_ptr,&preamble,sizeof(preamble_t));// atomic commit of the copied data
	return 1;
}


//iterate the buffer backwards and retrieve elements
checkpoint_t *log_read(log_t *log, char *var_name, int process_id,long version){
    checkpoint_t *rb_elem;

    if(log_isempty(log)){
        log_err("empty log buffer");
        return NULL;
    }
    ulong iter_index = log->ring_buffer.head->head;
    ulong tail_index = log->ring_buffer.head->tail;

	do{
        iter_index = (iter_index == 0)?(RING_BUFFER_SLOTS-1):(iter_index-1); // head point to next available slot. hence subtract first
        rb_elem = ringb_element(log,iter_index);
        if(!strncmp(rb_elem->var_name,var_name,VAR_SIZE) && rb_elem->version == version &&
                rb_elem->process_id == process_id){
            return rb_elem;
        }

    }while(tail_index != iter_index);

    log_warn("[%d] no checkpointed data found %s, %lu ",log->runtime_context->process_id,var_name,version);
    return NULL;

}



checkpoint_t* ringb_element(log_t *log, ulong index){
    assert(index < RING_BUFFER_SLOTS );
    return (((checkpoint_t*)(log->ring_buffer.elem_start_ptr)) + index);
}

void* log_ptr(log_t *log,ulong offset){
    assert(offset>=0 && offset < log->data_log.log_size );
    return ((char *)(log->data_log.start_ptr)+offset);
}



/* check whether our checkpoint init flag file is present */
int is_chkpoint_present(log_t *log){
	if(log->runtime_context->config_context->restart_run){
		return 1;
	}
	return 0;
}

/*
 * return 1-  empty
 *        0 - not empty
 */
int log_isempty(log_t *log){
    if(log->ring_buffer.head->tail == log->ring_buffer.head->head){
        return 1;
    }
    return 0;
}

/*
 * return 1 - full
 *        0 - not full
 */
int log_isfull(log_t *log){
    if(log->ring_buffer.head->tail == ((log->ring_buffer.head->head +1)%RING_BUFFER_SLOTS)){
        return 1;
    }
    return 0;
}

/*
 * init map
 */
static void init_mmap_files(log_t *log){
	int fd;
	void *addr;

    //init ring buffer file
    fd = open (log->ring_buffer.file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    ulong rblog_size = sizeof(headmeta_t) + sizeof(checkpoint_t)*RING_BUFFER_SLOTS;

    lseek (fd, rblog_size, SEEK_SET);
    write (fd, "", 1);
    lseek (fd, 0, SEEK_SET);
    addr = mmap (NULL,rblog_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){
        log_err("Runtime Error: error while memory mapping the file\n");
        exit(1);
    }

    debug("mapped the file: %s \n", log->ring_buffer.file_name);
    log->ring_buffer.head = addr;
    log->ring_buffer.elem_start_ptr =(checkpoint_t *)(((headmeta_t *)log->ring_buffer.head)+1);
    close (fd);

    //init data log memory map file
    fd = open (log->data_log.file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    lseek (fd, log->data_log.log_size, SEEK_SET);
    write (fd, "", 1);
    lseek (fd, 0, SEEK_SET);
    addr = mmap (NULL,log->data_log.log_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){
        log_err("Runtime Error: error while memory mapping the file\n");
        exit(1);
    }
    debug("mapped the file: %s \n", log->data_log.file_name);
    log->data_log.start_ptr = addr;
    close (fd);

	
	if(!log->runtime_context->config_context->restart_run){

        debug("first run of the library. Initializing the mapped files..\n");
        headmeta_t head;
        head.head = 0; // use the index to see if log is empty
        head.tail = 0;
        head.nelements = RING_BUFFER_SLOTS;
        head.log_size = log->data_log.log_size;
        memcpy(log->ring_buffer.head, &head, sizeof(headmeta_t));
        //TODO: introduce a magic value to check atomicity

        log->runtime_context->checkpoint_version = 0;
	}else{
        // TODO : implement

    }
}

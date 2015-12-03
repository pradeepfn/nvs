#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h>

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

void log_init(log_t *log , long log_size, int process_id){
	if(isDebugEnabled()){
		printf("initializing the structures... %d \n", process_id);
	}
    pthread_mutex_init(&(log->plock),NULL);
	log->data_log.log_size = log_size;
    snprintf(log->ring_buffer.file_name, sizeof(log->ring_buffer.file_name), "%s%s%d",pfile_location,FILE_PATH_ONE,process_id);
    snprintf(log->data_log.file_name, sizeof(log->data_log.file_name),"%s%s%d",pfile_location,FILE_PATH_TWO,process_id);
    init_mmap_files(log);
}

//update the current latest version of the log buffer and move the tail forward
int log_commitv(log_t *log,ulong version){
    pthread_mutex_lock(&log->plock);
    log->ring_buffer.head->current_version = version; // atomic commit
    //TODO: flush to fs
    //iterate starting from the tail and select a new tail
    checkpoint_t *rb_elem = ringb_element(log,log->ring_buffer.head->tail);
    while(rb_elem->version < (version-1)){ //less than two versions of current version
        log->ring_buffer.head->tail = (log->ring_buffer.head->tail+1)%RING_BUFFER_SLOTS;
        log->ring_buffer.log_tail = rb_elem->start_offset;
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

    debug("[%d] nvram checkpoint  varname : %s , process_id :  %d , version : %ld ",
          lib_process_id, s->varname, s->process_id, version);

    ulong checkpoint_size = sizeof(struct preamble_t_) + variable->size;

    //get the log reservation
    pthread_mutex_lock(&(log->plock));
    //check if slots left in the ring buffer
    if(log->ring_buffer.head->tail == ((log->ring_buffer.head->head+1)%RING_BUFFER_SLOTS)){
        debug("no slots left in the ring buffer")
        pthread_mutex_unlock(&(log->plock));
        return -1;
    }

    ulong dhead_offset = log->ring_buffer.log_head; // data head offset
    ulong dtail_offset = log->ring_buffer.log_tail; // data tail offset
    ulong dlog_size = log->ring_buffer.head->log_size; // data log size

    //if head is infront,
    if(dhead_offset > dtail_offset){
        avail_size = dlog_size - (dhead_offset+1);
        if(avail_size >= checkpoint_size){  // head to end of linear log
            reserved_log_offset = dhead_offset+1;
        }else if(dtail_offset >= checkpoint_size){   // start of the linear log to tail
            reserved_log_offset = 0;
        }else{
            debug("not enough space left in the linear log")
            pthread_mutex_unlock(&(log->plock));
            return -1;
        }

    }else if(dtail_offset > dhead_offset){
        avail_size = dtail_offset - (dhead_offset +1);
        if(avail_size >= checkpoint_size){
            reserved_log_offset = dhead_offset +1;
        }else{
            debug("not enough space in the linear log")
            pthread_mutex_unlock(&(log->plock));
            return -1;
        }
    }
    // we are good to do the final reserve...
    ulong next_rb_index = ((log->ring_buffer.head->head + 1)%RING_BUFFER_SLOTS);
    checkpoint_t *checkpoint_elem = ringb_element(log,next_rb_index);

    strncpy(checkpoint_elem->var_name,variable->varname,VAR_SIZE);
    checkpoint_elem->process_id = process_id;
    checkpoint_elem->version = version;
    checkpoint_elem->size = variable->size;
    checkpoint_elem->start_offset = reserved_log_offset;
    checkpoint_elem->hash = 0; // yet to implement
    checkpoint_elem->end_offset = reserved_log_offset + checkpoint_size;

    log->ring_buffer.head->head = next_rb_index; // atomically commit slot reservation
    pthread_mutex_unlock(&(log->plock));

    //non locked operation
    reserved_log_ptr = log_ptr(log,reserved_log_offset);
    preamble_log_ptr = log_ptr(log,checkpoint_elem->end_offset+1);
    preamble_t preamble;
    memcpy(preamble_log_ptr,&preamble,sizeof(preamble_t));
    // write to log on the granted log boundry and update the commit bit
    // valid_bit followed by data. we use the valid bit to atomically copy the checkpoint data.
    nvmmemcpy_write(reserved_log_ptr,variable->ptr,variable->size,nvram_wbw);
    ((preamble_t *)preamble_log_ptr)->value = MAGIC_VALUE; // atomic commit of the copied data
	return 1;
}


//iterate the buffer backwards and retrieve elements
checkpoint_t *log_read(log_t *log, char *var_name, int process_id,long version){
    checkpoint_t *rb_elem;
    ulong iter_index = log->ring_buffer.head->head;
    ulong tail_index = log->ring_buffer.head->tail;

	while(tail_index != iter_index){
        rb_elem = ringb_element(log,iter_index);
        if(!strncmp(rb_elem->var_name,var_name,VAR_SIZE && rb_elem->version == version &&
                rb_elem->process_id == process_id)){
            return rb_elem;
        }
        iter_index = (iter_index == 0)?(RING_BUFFER_SLOTS-1):(iter_index-1);
    }
    log_warn("[%d] no checkpointed data found %s, %lu ",lib_process_id,var_name,version);
    return NULL;

}



checkpoint_t* ringb_element(log_t *log, ulong index){
    assert(index>0 && index < RING_BUFFER_SLOTS );
    return (((checkpoint_t*)(log->ring_buffer.elem_start_ptr)) + index);
}

void* log_ptr(log_t *log,ulong offset){
    assert(offset>0 && offset < log->data_log.log_size );
    return ((char *)(log->data_log.start_ptr)+offset);
}



/* check whether our checkpoint init flag file is present */
int is_chkpoint_present(log_t *log){
	if(restart_run){
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
        printf("Runtime Error: error while memory mapping the file\n");
        exit(1);
    }

    debug("mapped the file: %s \n", log->ring_buffer.file_name);
    log->ring_buffer.head = addr;
    log->ring_buffer.elem_start_ptr =(checkpoint_t *)((headmeta_t *)log->ring_buffer.elem_start_ptr)+1;
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

	
	if(!restart_run){

        debug("first run of the library. Initializing the mapped files..\n");
        headmeta_t head;
        head.head = 0;
        head.tail = 0;
        head.current_version = -1;
        head.nelements = RING_BUFFER_SLOTS;
        head.log_size = log->data_log.log_size;
        memcpy(log->ring_buffer.head, &head, sizeof(headmeta_t));
        //TODO: introduce a magic value to check atomicity

        checkpoint_version = 0;
	}else{
        // TODO : implement

    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <pthread.h>

#include "px_log.h"
#include "px_debug.h"
#include "px_constants.h"
#include "px_util.h"
#include "phoenix.h"

#define FILE_PATH_ONE "/mmap.file.meta" // stores the ring buffer
#define FILE_PATH_TWO "/mmap.file.data" // stores linear metadata


int is_chkpoint_present(log_t *log);
static void init_mmap_files(log_t *log);

void* log_ptr(log_t *log,ulong offset);
ulong log_start_offset(log_t *log,ulong rb_index);
ulong log_end_offset(log_t *log,ulong rb_index);

int initshmlock(log_t *log, ccontext_t *configctxt){
	int fd;
	debug("lock file name : %s", configctxt->lckfile);
	fd = open(configctxt->lckfile,O_RDONLY);
	check(fd != -1 , "error opening file");

	check(flock(fd,LOCK_EX) != -1 , "error aquiring lock");
	if(!log->ring_buffer.head->log_initialized){
		//pthread_mutexattr_t mutexAttr;
		//pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
		//pthread_mutex_init(&(log->ring_buffer.head->plock),&mutexAttr);
		sem_init(&log->ring_buffer.head->sem,2,1);
		debug("[%d]shared semaphore initialized",log->runtime_context->process_id);
		log->ring_buffer.head->log_initialized = 1;
	}else{
		debug("shared semaphore already initialized");
	}
	check(flock(fd,LOCK_UN) != -1, "error releasing lock");
	return 0;
error:
	debug("error while initializing shared mem");
	exit(1);
}




/*  - mmap the  named files in to process adress space.
 *  - initialize the shared locks. -- use file locking as bootstrap locking mechanism
 */
int log_init(log_t *log , int proc_id){
	ccontext_t *config_context = log->runtime_context->config_context;

	log->data_log.log_size = config_context->log_size;
	snprintf(log->ring_buffer.file_name, sizeof(log->ring_buffer.file_name), "%s%s%d",
			config_context->pfile_location,FILE_PATH_ONE, proc_id);
	snprintf(log->data_log.file_name, sizeof(log->data_log.file_name),"%s%s%d",
			config_context->pfile_location,FILE_PATH_TWO, proc_id);
	init_mmap_files(log);

	/* init shared pthread locks */
	initshmlock(log,config_context);

	debug("[%d] log initialized successfully.", proc_id);
	return 0;
}



//update the current latest version of the log buffer and move the tail forward
int log_commitv(log_t *log,ulong version){
	//TODO: lock
	log->ring_buffer.head->current_version = version; // atomic commit
	//TODO: flush to fs
	//iterate starting from the tail and select a new tail
	checkpoint_t *rb_elem = ringb_element(log,log->ring_buffer.head->tail);
	while(rb_elem->version <= (version-1)){
		log->ring_buffer.head->tail = (log->ring_buffer.head->tail+1)%RING_BUFFER_SLOTS;
		rb_elem = ringb_element(log,log->ring_buffer.head->tail);
	}
	return 1;
}


/* writing to the data to persistent storage*/

int log_write(log_t *log, var_t *variable, long version){
	//debug("[%d] calling log write for variable %s", log->runtime_context->process_id,variable->key1);
	void *reserved_log_ptr,*preamble_log_ptr;
	ulong reserved_log_offset;
	ulong avail_size;
	ulong dhead_offset;
	ulong dtail_offset;

#ifdef DEDUP
	long dedup_varsize = get_varsize(variable->dedup_vector,variable->dv_size);
	ulong checkpoint_size = dedup_varsize + sizeof(struct preamble_t_) + variable->dv_size*sizeof(int);
#else
	ulong checkpoint_size = variable->size + sizeof(struct preamble_t_);
#endif
	//get the log reservation
	//pthread_mutex_lock(log->plock);
	//check if slots left in the ring buffer
	/*if(log_isfull(log)){
	  log_err("no slots left in the ring buffer");
	//pthread_mutex_unlock(log->plock);
	exit(1);
	}*/
	if(sem_wait(&log->ring_buffer.head->sem)== -1){
		log_err("error while sem wait");
		exit(1);
	}
	if(log->ring_buffer.head->head == -1 && log->ring_buffer.head->tail == -1){
		log->ring_buffer.head->head =0;
		log->ring_buffer.head->tail =0;
		dhead_offset = 0;
		dtail_offset = 0;
	}else{
		dhead_offset = log_end_offset(log, log->ring_buffer.head->head - 1); // data head offset , point to the next free head element to write
		dtail_offset = log_start_offset(log, log->ring_buffer.head->tail); // data tail offset , point to tail of the data log. if equal to head, then empty
	}
	ulong dlog_size = log->ring_buffer.head->log_size; // data log size
	/*debug("[%d] writing object : %s  with size : %ld, version: %ld   in to log, head_offset : %ld ,  tail_offset : %ld , log_size : %ld" ,
	  log->runtime_context->process_id,
	  variable->key1 ,
	  variable->size,
	  version,
	  dhead_offset,
	  dtail_offset,
	  dlog_size); */

	if(dhead_offset >= dtail_offset){   //if head is infront,
		avail_size = dlog_size - dhead_offset;
		if(avail_size >= checkpoint_size){  // head to end of linear log
			reserved_log_offset = dhead_offset;
		}else if(dtail_offset >= checkpoint_size){   // start of the linear log to tail
			reserved_log_offset = 0;
		}else{
			//pthread_mutex_unlock(log->plock);
			if(sem_post(&log->ring_buffer.head->sem) == -1){
				log_err("error while sem post");
				exit(1);
			}
			/*log_info("[%d]not enought space log tail : %ld ,  head : %ld , available_size : %ld " ,
			  log->runtime_context->process_id,
			  dtail_offset,
			  dhead_offset,
			  avail_size);
			  log_info("[%d]ring buffer indexes , %ld  %ld",
			  log->runtime_context->process_id,
			  log->ring_buffer.head->tail,
			  log->ring_buffer.head->head);*/
			return -1;
		}

	}else if(dtail_offset > dhead_offset){  // head is behind
		avail_size = dtail_offset - dhead_offset;
		if(avail_size >= checkpoint_size){
			reserved_log_offset = dhead_offset;
		}else{
			//pthread_mutex_unlock(log->plock);
			if(sem_post(&log->ring_buffer.head->sem) == -1){
				log_err("error while sem post");
				exit(1);
			}
			/*log_info("[%d]not enough space: log tail : %ld ,  head : %ld , available_size : %ld " ,
			  log->runtime_context->process_id,
			  dtail_offset,
			  dhead_offset,
			  avail_size);
			  log_info("[%d]ring buffer indexes , %ld  %ld",
			  log->runtime_context->process_id,
			  log->ring_buffer.head->tail,
			  log->ring_buffer.head->head);*/
			return -1;
		}
	}
	// we are good to do the final reserve...
	ulong rb_index = log->ring_buffer.head->head;

	checkpoint_t *checkpoint_elem = ringb_element(log,rb_index);

	strncpy(checkpoint_elem->var_name,variable->key1,KEY_LENGTH);
	//checkpoint_elem->process_id = process_id;
	checkpoint_elem->version = version;
	checkpoint_elem->size = variable->size;
	checkpoint_elem->start_offset = reserved_log_offset;
	checkpoint_elem->end_offset = reserved_log_offset + checkpoint_size-1;
#ifdef DEDUP
	checkpoint_elem->dv_size = variable->dv_size * sizeof(int);
#endif
	//debug("chekcpoint size : start offset : end offset of element = %ld :  %ld : %ld",
	//			checkpoint_size, checkpoint_elem->start_offset, checkpoint_elem->end_offset);

	log->ring_buffer.head->head = (log->ring_buffer.head->head + 1)%RING_BUFFER_SLOTS; // atomically commit slot reservation
	//pthread_mutex_unlock(log->plock);
	if(sem_post(&log->ring_buffer.head->sem) == -1){
		log_err("error while sem post");
		exit(1);
	}

#ifdef PX_DIGEST
	md5_digest(checkpoint_elem->hash,variable->ptr,variable->size);
	memcpy(variable->hash,checkpoint_elem->hash,MD5_LENGTH);
#endif

#ifdef DEDUP
	reserved_log_ptr = log_ptr(log,reserved_log_offset);
	ulong data_offset = reserved_log_offset + variable->dv_size*sizeof(int);
	void *data_ptr = log_ptr(log,data_offset);
	ulong preamble_offset = data_offset + variable->size;
	preamble_log_ptr = log_ptr(log,preamble_offset);

#else
	reserved_log_ptr = log_ptr(log,reserved_log_offset);
	ulong preamble_offset = reserved_log_offset + variable->size;
	preamble_log_ptr = log_ptr(log,preamble_offset);
#endif

#ifdef DEDUP
	// 1. write the dedup vector
	// 2. write the data in to persistent log after in page chunks
	nvmmemcpy_write(reserved_log_ptr,variable->dedup_vector,variable->dv_size*sizeof(int),
			log->runtime_context->config_context->nvram_wbw);

	int i;
	int chunk_started=0;
	int chunk_ended=0;
	long chunk_size=0;
	for(i=0; i<variable->dv_size;i++){
		if(!chunk_started){
			if(variable->dedup_vector[i]){
				chunk_started =1;
				chunk_size++;
			}else{
				continue;
			}

		}else if(chunk_started){
			if(variable->dedup_vector[i]){
				chunk_size++;
			}else{ // write the chunk
				chunk_started=0;
				chunk_size=0;
				nvmmemcpy_write(data_ptr,variable);
			}
		}
	}


#else
	// write to log on the granted log boundry and update the commit bit
	// valid_bit followed by data. we use the valid bit to atomically copy the checkpoint data.
	nvmmemcpy_write(reserved_log_ptr,variable->ptr,variable->size,log->runtime_context->config_context->nvram_wbw);
#endif
	preamble_t preamble;
	//preamble.value = MAGIC_VALUE;
	memcpy(preamble_log_ptr,&preamble,sizeof(preamble_t));// atomic commit of the copied data
	//debug("[%d] done writing the variable %s " , log->runtime_context->process_id, variable->key1);
	return 1;
}


//iterate the buffer backwards and retrieve elements
checkpoint_t *log_read(log_t *log, char *key, long version){
	checkpoint_t *rb_elem;

	if(log_isempty(log)){
		log_err("empty log buffer");
		return NULL;
	}

	//pthread_mutex_lock(log->plock);
	ulong iter_index = log->ring_buffer.head->head;
	ulong tail_index = log->ring_buffer.head->tail;
	//pthread_mutex_unlock(log->plock);

	do{
		iter_index = (iter_index == 0)?(RING_BUFFER_SLOTS-1):(iter_index-1); // head point to next available slot. hence subtract first
		rb_elem = ringb_element(log,iter_index);
		if(!strncmp(rb_elem->var_name,key,KEY_LENGTH) && rb_elem->version == version){
			return rb_elem;
		}

	}while(tail_index != iter_index);

	log_warn("[%d] no checkpointed data found %s, %lu ",log->runtime_context->process_id,key,version);
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

ulong log_start_offset(log_t *log,ulong rb_index){
	checkpoint_t *rb_elem = ringb_element(log, rb_index);
	return rb_elem->start_offset;
}

ulong log_end_offset(log_t *log, ulong rb_index){
	checkpoint_t *rb_elem = ringb_element(log, rb_index);
	return rb_elem->end_offset;
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

	//pthread_mutex_lock(log->plock);
	if(sem_wait(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem wait");
		exit(1);
	}
	if(log->ring_buffer.head->tail == log->ring_buffer.head->head){
		//pthread_mutex_unlock(log->plock);
		if(sem_post(&log->ring_buffer.head->sem) == -1){
			log_err("error in sem wait");
			exit(1);
		}
		return 1;
	}
	//pthread_mutex_unlock(log->plock);
	if(sem_post(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem wait");
		exit(1);
	}
	return 0;
}

/*
 * return 1 - full
 *        0 - not full
 */
int log_isfull(log_t *log){
	//pthread_mutex_lock(log->plock);
	if(log->ring_buffer.head->tail == ((log->ring_buffer.head->head +1)%RING_BUFFER_SLOTS)){
		//pthread_mutex_unlock(log->plock);
		return 1;
	}
	//pthread_mutex_unlock(log->plock);
	return 0;
}


int create_shm(char *ishm_name, char *dshm_name,ulong log_size){
	int fd;
	void *dptr,*iptr;

	//create and format index shm file
	fd = open (ishm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	ulong rblog_size = sizeof(headmeta_t) + sizeof(checkpoint_t)*RING_BUFFER_SLOTS;

	lseek (fd, rblog_size, SEEK_SET);
	write (fd, "", 1);
	lseek (fd, 0, SEEK_SET);
	iptr = mmap (NULL,rblog_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if(iptr == MAP_FAILED){
		log_err("Memory map failure\n");
		exit(1);
	}

	//create and format data shm file
	fd = open (dshm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, log_size, SEEK_SET);
	write (fd, "", 1);
	lseek (fd, 0, SEEK_SET);
	dptr = mmap (NULL,log_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if(dptr == MAP_FAILED){
		log_err("Runtime Error: error while memory mapping the file\n");
		exit(1);
	}
	headmeta_t head;
	head.head = -1; // we identify the start condition from initial value
	head.tail = -1;
	head.nelements = RING_BUFFER_SLOTS;
	head.log_size = log_size;
	head.log_initialized = 0;
	memcpy(iptr, &head, sizeof(headmeta_t));

	return 0;
}



/*
 * init map
 */
static void init_mmap_files(log_t *log){
	int fd;
	void *addr;

	//map the named shm
	fd = open (log->ring_buffer.file_name, O_RDWR, S_IRUSR | S_IWUSR);
	ulong rblog_size = sizeof(headmeta_t) + sizeof(checkpoint_t)*RING_BUFFER_SLOTS;

	addr = mmap (NULL,rblog_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED){
		log_err("Memory map failure\n");
		exit(1);
	}

	debug("mapped the file: %s \n", log->ring_buffer.file_name);
	log->ring_buffer.head = addr;
	log->ring_buffer.elem_start_ptr =(checkpoint_t *)(((headmeta_t *)addr)+1);
	close (fd);

	//init data log memory map file
	fd = open (log->data_log.file_name, O_RDWR, S_IRUSR | S_IWUSR);
	addr = mmap (NULL,log->data_log.log_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED){
		log_err("Runtime Error: error while memory mapping the file\n");
		exit(1);
	}
	debug("mapped the file: %s \n", log->data_log.file_name);
	log->data_log.start_ptr = addr;
	close (fd);


}

/* perform cleanup */
int log_finalize(log_t *log){

	if(sem_wait(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem wait");
		exit(1);
	}
	log->ring_buffer.head->log_initialized = 1;
	if(sem_post(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem post");
		exit(1);
	}
	//destroy semaphore
	if(sem_destroy(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem destroy");
		exit(1);
	}
	return 0;
}

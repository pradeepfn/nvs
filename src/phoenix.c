#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <semaphore.h>
#include <unistd.h>

#include "phoenix.h"
#include "px_log.h"
#include "px_debug.h"
#include "px_util.h"
#include "px_constants.h"



/* set some of the variables on the stack */
rcontext_t runtime_context;
ccontext_t config_context;
var_t *varmap = NULL;
log_t nvlog;

/* local variables */
int lib_initialized = 0;

int px_init(int proc_id){
	//tie up the global variable hieararchy
	runtime_context.config_context = &config_context;
	runtime_context.varmap = &varmap;
	runtime_context.checkpoint_version=0;
	runtime_context.process_id = proc_id;
	nvlog.runtime_context = &runtime_context;

	if(lib_initialized){
		log_err("Error: the library already initialized.");
		exit(1);
	}
	read_configs(&config_context,CONFIG_FILE_NAME);
	log_init(&nvlog,proc_id);
	return 0;
}



/**
 * create an object in volatile memory. The runtime only remembers
 * the object once the object gets to persistence memory/storage
 */
int px_create(char *key1, unsigned long size,px_obj *retobj){
	var_t *s;
	s = px_alighned_allocate(size, key1);
	HASH_ADD_STR(varmap, key1, s );

	retobj->data = s->ptr;
	retobj->size = size;

	return 0; // success
}




/**
 * return the data pointed by the key. If the data is not found in fast memory,
 * then move the data to fast memory from slow memory before returning the pointer.
 */
int px_get(char *key1, uint64_t version, px_obj *retobj){
	var_t *s;
	// return from the in-memory store
	for(s=varmap; s != NULL; s=s->hh.next){
		if(!strncmp(s->key1, key1, KEY_LENGTH)){
			retobj->data = s->ptr;
			retobj->size = s->size;
			return 0;
		}
	}

	// if a specific version specified, then get it from the nv-store
	retobj->data = NULL;
	retobj->size = 0;
	return 0;
}



/*
 * Move the data from volatile memory to non volatile log structured memory.
 * TODO: how version returning works?
 */
int px_commit(char *key1,int version) {
	var_t *s;
	for (s = varmap; s != NULL; s = s->hh.next){
		if(!strncmp(s->key1,key1,KEY_LENGTH)){
			log_write(&nvlog, s, version);
		}
	}
	return 0;
}


int px_snapshot(){
	//debug("[%d] creating snapshot with version %ld",runtime_context.process_id,runtime_context.checkpoint_version);
	var_t *s;
	for (s = varmap; s != NULL; s = s->hh.next){
		while(log_write(&nvlog, s, runtime_context.checkpoint_version) == -1){ //not enough space
			sleep(0.5);
	    };
	}
	runtime_context.checkpoint_version ++;
	return 0;
}

int read_watermark=0;

/*read the next most recent snapshot */
int px_get_snapshot(ulong version){
    log_t *log = &nvlog;


	// check if log empty
	while(log_isempty(log)){ sleep(0.5);};
	debug("waiting for semaphore");
    if(sem_wait(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem wait");
		exit(1);
    }
    checkpoint_t *rb_elem = ringb_element(log,log->ring_buffer.head->tail);
	//truncating log
	debug("traversing log");
	while(log->ring_buffer.head->tail != log->ring_buffer.head->head && rb_elem->version <= version){ 
        log->ring_buffer.head->tail = (log->ring_buffer.head->tail+1)%RING_BUFFER_SLOTS;
        rb_elem = ringb_element(log,log->ring_buffer.head->tail);
    }

	debug("posting to semaphore");
    if(sem_post(&log->ring_buffer.head->sem) == -1){
		log_err("error in sem wait");
		exit(1);
    }
	debug("snapshot data returned version : %ld" , version);
	return 0;
}

/**
 *	free the dram memory allocation
 */
int px_delete(char *key1){
	var_t *s;

	for (s = varmap; s != NULL; s = s->hh.next){
		if(strncmp(s->key1, key1,KEY_LENGTH)){
			free(s->ptr);
		}
	}
	return 0;
}



int px_finalize(){
	log_finalize(&nvlog);
	return 0;
}

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
 * we search the object in persistence store/ NVRAM as this get operation is
 * for consumers.
 * 1. we write protect the object before return
 * 2. remember the return object and version
 * 3. apply the diff for next version
 */
int px_get(char *key1, uint64_t version, px_obj *retobj){
	checkpoint_t *objmeta = log_read(&nvlog, key1, version);
	if(objmeta != NULL){
		void *ptr = log_ptr(&nvlog,objmeta->start_offset);
		void *rptr = malloc(objmeta->size);
		// application is responsible for freeing up object
		memcpy(rptr,ptr,objmeta->size);
		retobj->data = rptr;
		retobj->size = objmeta->size;
		return 0;
	}else{
		log_err("key not found : %s", key1);
		return -1;
	}
}


/* apply the diff to given version
 *  -- currently we only support consercative version
 */
int px_deltaget(char *key1,uint64_t version, px_obj *retobj){
#ifdef DEDUP
	void *dataptr;
	int  *vectorptr;
	checkpoint_t *objmeta = log_read(&nvlog, key1, version);
	if(objmeta != NULL){
		vectorptr = (int *)log_ptr(&nvlog,objmeta->start_offset);
		dataptr = log_ptr(&nvlog,objmeta->start_offset + objmeta->dv_size*sizeof(int));
	}else{
		log_err("key not found : %s", key1);
		return -1;
	}
	if(version == 0){
		// for now we consider this as a case. Fix this later!
		retobj->data = malloc(objmeta->size);
		retobj->size = objmeta->size;
	}else{
		assert(retobj->data != NULL);
		assert(retobj->version == version -1);
	}

	char str[128];
	int k;
	int index = 0;
	for (k=0; k<objmeta->dv_size; k++){
		index += snprintf(&str[index], 128-index, "%d ", vectorptr[k]);
	}
	debug("dedup string : %s", str);

	long temp = nvmmemcpy_dedup_apply(retobj->data, retobj->size, dataptr,objmeta->size,
			vectorptr,objmeta->dv_size);
	debug("dedup obj size : %ld , applied data size ; %d", objmeta->dedup_size, temp);
	assert(objmeta->dedup_size == temp);
	retobj->version = objmeta->version;
	retobj->size = objmeta->size;
	return 0;
#else

	return px_get(key1,version,retobj);

#endif
}

/*
 * Move the data from volatile memory to non volatile log structured memory.
 * TODO: how version returning works?
 */
int px_commit(char *key1,int version) {
	var_t *s;
	for (s = varmap; s != NULL; s = (var_t *)s->hh.next){
		if(!strncmp(s->key1,key1,KEY_LENGTH)){
			log_write(&nvlog, s, version);
#ifdef DEDUP
			//memset the dedup vector and mprotect pages
			memset(s->dedup_vector,0,s->dv_size*sizeof(int));
			enable_write_protection(s->ptr, s->paligned_size);
#endif
			return 0;
		}
	}
	log_err("key not found\n");
	return -1;
}


int px_snapshot(){
	if(!runtime_context.process_id){
		log_info("[%d] px_snapshot.", runtime_context.process_id);
	}
	//debug("[%d] creating snapshot with version %ld",runtime_context.process_id,runtime_context.checkpoint_version);
	var_t *s;
	for (s = varmap; s != NULL; s = (var_t *) s->hh.next){
		log_write(&nvlog, s, runtime_context.checkpoint_version);
#ifdef DEDUP
		if(runtime_context.checkpoint_version){

			print_dedup_numbers(s, runtime_context.checkpoint_version);
		}
		//memset the dedup vector and mprotect pages
		memset(s->dedup_vector,0,s->dv_size*sizeof(int));
		enable_write_protection(s->ptr, s->paligned_size);
#endif
	}
	runtime_context.checkpoint_version ++;
	return 0;
}


/*read the next most recent snapshot */
int px_get_snapshot(ulong version){
	// for now we implement the get snapshot functionality in the library itself.
	var_t *s;
	for (s = varmap; s != NULL; s = (var_t *)s->hh.next){
#ifdef DEDUP
		px_deltaget(s->key1, version, &(s->cobj));
#else
		px_get(s->key1, version, &(s->cobj));
		// free the object
#endif
	}
	return 0;
}

/**
 *	free the dram memory allocation
 */
int px_delete(char *key1){
	var_t *s;

	for (s = varmap; s != NULL; s = (var_t *)s->hh.next){
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

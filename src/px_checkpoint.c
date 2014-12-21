#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "phoenix.h"
#include "px_checkpoint.h"
#include "px_log.h"
#include "px_read.h"
#include "px_debug.h"

#define LOG_SIZE 1000000
#define CHUNK_SIZE 4096
#define CONFIG_FILE_NAME "phoenix.config"
#define NVRAM_SIZE "nvramcapacity"

int is_remaining_space_enough(int);

long log_size = -1;
int lib_initialized = 0;
int chunk_size;

log_t chlog;
listhead_t head;


void init(int process_id){
	// these configs should get load from the config file 
	log_size = LOG_SIZE;	
	chunk_size = CHUNK_SIZE;
	log_init(&chlog,log_size,process_id);
	LIST_INIT(&head);
}


void *alloc(char *var_name, size_t size, size_t commit_size,int process_id){
    //initialize the library
    if(!lib_initialized){
        init(process_id);
        lib_initialized = 1;
    }   

    entry_t *n = malloc(sizeof(struct entry)); // new node in list
    memcpy(n->var_name,var_name,VAR_SIZE);
    n->size = size;
    n->process_id = process_id;
    n->version = 0;

	//if checkpoint data present, then read from the the checkpoint
	if(is_chkpoint_present(&chlog)){
		if(isDebugEnabled()){
			printf("retrieving from the checkpointed memory : %s\n", var_name);
		}
        //n->ptr = copy_read(&chlog, var_name,process_id);
        n->ptr = fault_read(&chlog, var_name,process_id);
	}else{
		if(isDebugEnabled()){
			printf("allocating from the heap space\n");
		}
        n->ptr = malloc(size);
	}
    LIST_INSERT_HEAD(&head, n, entries);
    return n->ptr;
}


void chkpt_all(int process_id){
	if(isDebugEnabled()){
		printf("checkpointing data of process : %d \n",process_id);
	}
	log_write(&chlog,&head,process_id);
    return;
}


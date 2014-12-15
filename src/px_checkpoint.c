#include "px_checkpoint.h"



int nvm_size = -1;
int lib_initializedi = 0;


void init(int process_id){



}


void *alloc(char *var_name, size_t size, size_t commit_size,int process_id){
    //initialize the library
    if(!lib_initialized){
        init(process_id);
        libg_initialized = 1;
    }   
    struct entry *n = malloc(sizeof(struct entry)); // new node in list
    memcpy(n->var_name,var_name,VAR_SIZE);
    n->size = size;
    n->process_id = process_id;
    n->version = 0;
	//if checkpoint data present, then read from the the checkpoint
	if(is_ckpt_available){
		if(isDebugEnabled()){
			printf("retrieving from the checkpointed memory : %s\n", var_name);
		}
        n->ptr = nvread(var_name,process_id);
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
    if(!is_remaining_space_enough(process_id)){
		printf("Runtime Error : allocated space is not enough for checkpointing\n");
		assert(0);	
    }   
    struct entry *np;
    for (np = head.lh_first; np != NULL; np = np->entries.le_next){
        if(np->process_id == process_id){ 
            checkpoint(np->var_name, np->process_id, np->version,   np->size,np->ptr);
        }
    }   
	// TODO : update the log head
    return;
}





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "phoenix.h"
#include "px_checkpoint.h"
#include "px_log.h"
#include "px_read.h"
#include "px_debug.h"

#define CONFIG_FILE_NAME "phoenix.config"

//config file params
#define NVM_SIZE "nvm.size"
#define CHUNK_SIZE "chunk.size"
#define COPY_STRATEGY "copy.strategy"
#define DEBUG_ENABLE "debug.enable"


//copy stategies
#define NAIVE_COPY 1
#define FAULT_COPY 2
#define PRE_COPY 3
#define FAST_RESTART 4


int is_remaining_space_enough(int);

/*default config params*/
long log_size = 2*1024*1024;
int chunk_size = 4096;
int copy_strategy = 1;
int lib_initialized = 0;

int lib_process_id = -1;
int  checkpoint_size_printed = 0; // flag variable
long checkpoint_size = 0;

log_t chlog;
listhead_t head;
tlisthead_t thead;

thread_t thread;


void init(int process_id){
	char varname[30];
	long varvalue;
	lib_process_id = process_id;
	
	//naive way of reading the config file
	FILE *fp = fopen(CONFIG_FILE_NAME,"r");
	if(fp == NULL){
		printf("error while opening the file\n");
		exit(1);
	}
	while (fscanf(fp, "%s =  %ld", varname, &varvalue) != EOF) {
		if(!strncmp("nvm.size",varname,sizeof(varname))){
			log_size = varvalue*1024*1024;		
		}else if(!strncmp("chunk.size",varname,sizeof(varname))){
			chunk_size = (int)varvalue;	
		}else if(!strncmp("copy.strategy",varname,sizeof(varname))){
			copy_strategy = (int)varvalue;	
		}else if(!strncmp(DEBUG_ENABLE,varname,sizeof(varname))){
			if(((int)varvalue) == 1){			//enable debug
				enable_debug();		
			}
		}else{
			printf("unknown varibale. please check the config\n");
			exit(1);
		}
	}
	fclose(fp);	
	if(isDebugEnabled()){
		printf("size of log in bytes : %ld\n", log_size);
		printf("chunk size in bytes : %d\n", chunk_size);
		printf("copy strategy is set to : %d\n", copy_strategy);
	}

	log_init(&chlog,log_size,process_id);
	LIST_INIT(&head);
	
}


void *alloc(char *var_name, size_t size, size_t commit_size,int process_id){
    //initialize the library
    if(!lib_initialized){
        init(process_id);
        lib_initialized = 1;
    }   

	//counting the total checkpoint data size per core
	checkpoint_size+= size;

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
	/*Different copy methods*/
		switch(copy_strategy){
			case 1:
				n->ptr = copy_read(&chlog, var_name,process_id);
				if(isDebugEnabled()){
					printf("copy strategy set to : naive read\n");
				}
				break;
			case 2:
				n->ptr = fault_read(&chlog, var_name,process_id);
				if(isDebugEnabled()){
					printf("copy strategy set to : fault read\n");
				}
				break;	
			case 3:
				n->ptr = pc_read(&chlog, var_name,process_id);
				if(isDebugEnabled()){
					printf("copy strategy set to : precopy read\n");
				}
				break;
			case 4:	
				//fall through
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}
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
	if(lib_process_id == 0 && !checkpoint_size_printed){ // if this is the MPI main process log the checkpoint size
		printf("checkpoint size : %.2f \n", checkpoint_size/1000000);
		checkpoint_size_printed = 1;
	}
	log_write(&chlog,&head,process_id);
    return;
}


void* alloc_(unsigned int* n, char *s, int *iid, int *cmtsize) {
	return alloc(s,*n,*cmtsize,*iid); 
}

void afree_(char* arr) {
	free(arr);
}

void chkpt_all_(int *process_id){
	chkpt_all(*process_id);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "phoenix.h"
#include "px_checkpoint.h"
#include "px_log.h"
#include "px_read.h"
#include "px_remote.h"
#include "px_debug.h"
#include "px_util.h"
#include "px_dlog.h"

#define CONFIG_FILE_NAME "phoenix.config"

//config file params
#define NVM_SIZE "nvm.size"
#define CHUNK_SIZE "chunk.size"
#define COPY_STRATEGY "copy.strategy"
#define DEBUG_ENABLE "debug.enable"
#define PFILE_LOCATION "pfile.location"
#define NVRAM_WBW "nvram.wbw"
#define RSTART "rstart"
#define REMOTE_CHECKPOINT_ENABLE "rmt.chkpt.enable"
#define REMOTE_RESTART_ENABLE "rmt.rstart.enable"


//copy stategies
#define NAIVE_COPY 1
#define FAULT_COPY 2
#define PRE_COPY 3
#define PAGE_ALIGNED_COPY 4
#define REMOTE_COPY 5
#define REMOTE_FAULT_COPY 6
#define REMOTE_PRE_COPY 7


int is_remaining_space_enough(int);


/*default config params*/
long log_size = 2*1024*1024;
int chunk_size = 4096;
int copy_strategy = 1;
int lib_initialized = 0;

int lib_process_id = -1;
int  checkpoint_size_printed = 0; // flag variable
long checkpoint_size = 0;
int nvram_wbw = -1;
int rstart = 0;
int remote_checkpoint = 0;
int remote_restart = 0;
char pfile_location[32];

int status; // error status register

log_t chlog;
dlog_t chdlog;
listhead_t head;
tlisthead_t thead;

thread_t thread;

int init(int proc_id, int nproc){
	char varname[30];
	char varvalue[32];// we are reading integers in to this
	if(lib_initialized){
		printf("Error: the library already initialized.");
		exit(1);
	}

	lib_process_id = proc_id;
	
	//naive way of reading the config file
	FILE *fp = fopen(CONFIG_FILE_NAME,"r");
	if(fp == NULL){
		printf("error while opening the file\n");
		exit(1);
	}
	while (fscanf(fp, "%s =  %s", varname, varvalue) != EOF) {
		if(varname[0] == '#'){
			// consuming the input line starting with '#'
			fscanf(fp,"%*[^\n]");  
			fscanf(fp,"%*1[\n]"); 
			continue;
		}else if(!strncmp(NVM_SIZE,varname,sizeof(varname))){
			log_size = atoi(varvalue)*1024*1024;		
		}else if(!strncmp(CHUNK_SIZE,varname,sizeof(varname))){
			chunk_size = atoi(varvalue);	
		}else if(!strncmp(COPY_STRATEGY,varname,sizeof(varname))){
			copy_strategy = atoi(varvalue);	
		}else if(!strncmp(DEBUG_ENABLE,varname,sizeof(varname))){
			if(atoi(varvalue) == 1){		
				enable_debug();		
			}
		}else if(!strncmp(PFILE_LOCATION,varname,sizeof(varname))){
			strncpy(pfile_location,varvalue,sizeof(pfile_location));
		}else if(!strncmp(NVRAM_WBW,varname,sizeof(varname))){
			nvram_wbw = atoi(varvalue);
		}else if(!strncmp(RSTART,varname,sizeof(varname))){
			rstart = atoi(varvalue);
		}else if(!strncmp(REMOTE_CHECKPOINT_ENABLE,varname,sizeof(varname))){
			if(atoi(varvalue) == 1){		
				remote_checkpoint = 1;		
			}
		}else if(!strncmp(REMOTE_RESTART_ENABLE,varname,sizeof(varname))){
			if(atoi(varvalue) == 1){		
				remote_restart = 1;		
			}
		}else {
			printf("unknown varibale. please check the config\n");
			exit(1);
		}
	}
	fclose(fp);	
	if(isDebugEnabled()){
		printf("size of log in bytes : %ld bytes\n", log_size);
		printf("chunk size in bytes : %d\n", chunk_size);
		printf("copy strategy is set to : %d\n", copy_strategy);
		printf("persistant file location : %s\n", pfile_location);
		printf("NVRAM write bandwidth : %d Mb\n", nvram_wbw);
	}
    if(remote_checkpoint || remote_restart){	
		status = remote_init(proc_id,nproc);
		if(status){printf("Error: initializing remote copy procedures..\n");}
	}
	log_init(&chlog,log_size,proc_id);
    dlog_init(&chdlog);
	LIST_INIT(&head);
    if(isDebugEnabled()){
        printf("phoenix initializing completed\n");
    }
	return 0;	
}


void *alloc(char *var_name, size_t size, size_t commit_size,int process_id){

	//counting the total checkpoint data size per core
	checkpoint_size+= size;

    entry_t *n = malloc(sizeof(struct entry)); // new node in list
    memcpy(n->var_name,var_name,VAR_SIZE);
    n->size = size;
    n->process_id = process_id;
    n->version = 0;

	//if checkpoint data present, then read from the the checkpoint
	if(remote_restart){

		//if(isDebugEnabled()){
		//	printf("retrieving from the remote checkpointed memory : %s\n", var_name);
		//}
		switch(copy_strategy){
		   	case REMOTE_COPY:	
				remote_copy_read(&chlog, var_name,get_mypeer(process_id),n);
				break;
			case REMOTE_FAULT_COPY:	
				remote_chunk_read(&chlog, var_name,get_mypeer(process_id),n);
				break;
			case REMOTE_PRE_COPY:	
				remote_pc_read(&chlog, var_name,get_mypeer(process_id),n);
				break;
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}

	}else if(is_chkpoint_present(&chlog)){
		if(isDebugEnabled()){
			printf("retrieving from the checkpointed memory : %s\n", var_name);
		}
	/*Different copy methods*/
		switch(copy_strategy){
			case NAIVE_COPY:
				n->ptr = copy_read(&chlog, var_name,process_id);
				break;
			case FAULT_COPY:
				n->ptr = chunk_read(&chlog, var_name,process_id);
				break;	
			case PRE_COPY:
				n->ptr = pc_read(&chlog, var_name,process_id);
				break;
			case PAGE_ALIGNED_COPY:	
				n->ptr = page_aligned_copy_read(&chlog, var_name,process_id);
				break;
			default:
				printf("wrong copy strategy specified. please check the configuration\n");
				exit(1);
		}
	}else{
		if(isDebugEnabled()){
			printf("allocating from the heap space\n");
		}
        //local memory space allocatoin
        n->ptr = malloc(size);
		memset(n->ptr,0,size);
        //remote memory group allocation of memory
        //TODO: FIXME: we dont allocation for all the variables
        n->local_ptr = remote_alloc(&n->rmt_ptr,size);
	}
    LIST_INSERT_HEAD(&head, n, entries);
    return n->ptr;
}

/*
 * The procedure is responsible for hybrid checkpoints. The phoenix version of local checkpoitns.
 * 1. First we mark what to checkpoint to local NVRAM/what goes to remote DRAM
 * 2. Parallelly starts;
 * 			- local NVRAM checkpoint - X portions of the Total checkpoint
 * 			- local DRAM checkpoint - (Total -X) portion of data
 * 			- remote DRAM checkpoint - (Total-X) portion of data
 * 	3. We are done with
 */
void chkpt_all(int process_id){
	entry_t *np;
    if(rstart == 1){
		printf("skipping checkpointing data of process : %d \n",process_id);
		return;
	}
	split_checkpoint_data(&head,process_id); // decide on what variable go where
    log_write(&chlog,&head,process_id);//local NVRAM write
    dlog_remote_write(&chdlog, &head,get_mypeer(process_id));//remote DRAM write
    dlog_local_write(&chdlog, &head,process_id);//local DRAM write

    if(lib_process_id == 0 && !checkpoint_size_printed){ // if this is the MPI main process log the checkpoint size
        printf("checkpoint size : %.2f \n", (double)checkpoint_size/1000000);
        checkpoint_size_printed = 1;
    }
    return;
}




void *alloc_(unsigned int *n, char *s, int *iid, int *cmtsize) {
	return alloc(s, *n, *cmtsize, *iid);
}

void afree_(void* ptr) {
	free(ptr);
}


void afree(void* ptr) {
	free(ptr);
}

void chkpt_all_(int *process_id){
	chkpt_all(*process_id);
}
int init_(int *proc_id, int *nproc){
	return init(*proc_id,*nproc);
}

int finalize_(){
  return remote_finalize();
}

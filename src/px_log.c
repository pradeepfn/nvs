#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "px_log.h"
#include "px_debug.h"
#include "px_util.h"

#define FILE_PATH_ONE "/mmap.file.one"
#define FILE_PATH_TWO "/mmap.file.two"

//global variables
int first_run=0;
extern char pfile_location[32];
extern int lib_process_id;
extern long checkpoint_version;

int is_chkpoint_present(log_t *log);
static void init_mmap_files(log_t *log);
static memmap_t *get_latest_mapfile(log_t *log);
static void checkpoint(log_t *log, char *var_name, int process_id, int version, size_t size, void *data,int wbw);
static void checkpoint1(log_t *log,void *start_addr, checkpoint_t *chkpt, void *data,int wbw);
static checkpoint_t *get_meta(void *base_addr,size_t offset);
static void *get_start_addr(void *base_addr,checkpoint_t *last_meta);
static int is_remaining_space_enough(log_t *log, var_t *list);
static int is_remaining_space_enough2(log_t *log, dcheckpoint_map_entry_t *map);


extern int nvram_wbw;
extern int nvram_ec_wbw;


void log_init(log_t *log , long log_size, int process_id){
	if(isDebugEnabled()){
		printf("initializing the structures... %d \n", process_id);
	}
    //init log guarding lock
    //pthread_mutex_init(&log->mtx,NULL);

	log->log_size = log_size/2;
    snprintf(log->m[0].file_name, sizeof(log->m[0].file_name), "%s%s%d",pfile_location,FILE_PATH_ONE,process_id);
    snprintf(log->m[1].file_name, sizeof(log->m[1].file_name),"%s%s%d",pfile_location,FILE_PATH_TWO,process_id);
    init_mmap_files(log);
}



/* writing to the data to persistent storage*/
extern long nvram_checkpoint_size;

int destage_data_log_write(log_t *log,dcheckpoint_map_entry_t *map,int process_id){
    //log_info("[%d] destage thread starting",lib_process_id);
    dcheckpoint_map_entry_t *s;
    if(is_remaining_space_enough2(log,map)){
        //go ahead and append to log
        for(s=map;s!=NULL;s=s->hh.next){
            if(s->process_id == process_id) {
                    //og_info("[%d] nvram destage checkpoint  varname : %s , process_id :  %d , version : %ld , size : %ld ,"
                     //              "pointer : %p \n", lib_process_id, s->var_name, s->process_id, s->version, s->size,
                     //      s->data_ptr);
                nvram_checkpoint_size+= s->size;
                checkpoint(log, s->var_name, process_id, s->version, s->size, s->data_ptr,nvram_wbw);
            }
        }
    }else{
        //allocate more space
        log_err("not enough space for data destaging..");
        assert(0);
    }
    //log_info("[%d] destage thread exiting",lib_process_id);
    return 1;
}

extern int early_copy_enabled;

int log_write(log_t *log, var_t *list, int process_id,long version){
	var_t *s;
	if(!is_remaining_space_enough(log, list)){
		if(isDebugEnabled()){ 
			printf("remaining space is not enough. Switching to other file....\n");
		}
		log->current = (log->current == &(log->m[0]))?&(log->m[1]):&(log->m[0]);	
		log->current->head->offset = -1; // invalidate the data
		gettimeofday(&(log->current->head->timestamp),NULL); // setting the timestamp
	}
    for (s = list; s != NULL; s = s->hh.next){
        //nvram bound, not early copied variable
        if(s->process_id == process_id && s->type == NVRAM_CHECKPOINT && (!s->early_copied) ){

				/*log_info("[%d] nvram checkpoint  varname : %s , process_id :  %d , version : %ld , size : %ld ,"
							"pointer : %p \n",lib_process_id, s->varname, s->process_id, version, s->size, s->ptr);*/
            debug("[%d] nvram checkpoint  varname : %s , process_id :  %d , version : %ld ",
                     lib_process_id, s->varname, s->process_id, version);

		    nvram_checkpoint_size+= s->size;
            checkpoint(log, s->varname, s->process_id, version, s->size, s->ptr,nvram_wbw);
        }

        if(early_copy_enabled && s->early_copied){
            disable_protection(s->ptr, s->size); // resetting the page protection
            s->early_copied = 0; // resetting the flag to next iteration
        }
    }	

	return 1;
}

/*
 * This log write function get used by the early copy thread
 */
int log_write_var(log_t *log, var_t *np, long version){
    //TODO : check for remaining space
    assert(np->type == NVRAM_CHECKPOINT);
    nvram_checkpoint_size+= np->size;
    checkpoint(log,np->varname,np->process_id,version,np->size,np->ptr,nvram_ec_wbw);
    return 1;
}



checkpoint_t *log_read(log_t *log, char *var_name, int process_id,long version){
	offset_t temp_offset = log->current->head->offset;
	checkpoint_t *cptr = log->current->meta;
    int str_cmp;
    while(temp_offset >= 0){ 
		struct timeval t1; 
		struct timeval t2; 
		gettimeofday(&t1,NULL);
        checkpoint_t *ptr = get_meta(cptr,temp_offset);
		gettimeofday(&t2,NULL);
		if(isDebugEnabled()){
			printf("[%d] comparing values  process ids (%d, %d) - (%s, %s)\n",lib_process_id ,ptr->process_id,
										 process_id,ptr->var_name,var_name);
		}
		if((ptr->process_id == process_id) && (str_cmp=strncmp(ptr->var_name,var_name,10))==0
                && version == ptr->version){ // last char is null terminator
			return ptr;
		}
		temp_offset = ptr->prv_offset;
	}   
    memmap_t *secondary  = (log->current == &(log->m[0]))?&(log->m[1]):&(log->m[0]);  
    temp_offset = secondary->head->offset;
    cptr = secondary->meta;
    while(temp_offset >= 0){ 
        checkpoint_t *ptr = get_meta(cptr,temp_offset);
        if(isDebugEnabled()){
            printf("[%d] [secondary log] comparing values  process ids (%d, %d) - (%s, %s)\n",lib_process_id ,ptr->process_id,
                                                                             process_id,ptr->var_name,var_name);
        }
        if((ptr->process_id == process_id) && (str_cmp=strncmp(ptr->var_name,var_name,10))==0
                && version == ptr->version){ // last char is null terminator
            return ptr;
        }
        temp_offset = ptr->prv_offset;
   }
    return NULL;//to-do, handle exception cases
}


/* check whether our checkpoint init flag file is present */
int is_chkpoint_present(log_t *log){
	if(!first_run){
		return 1;
	}
	return 0;
}



static void init_mmap_files(log_t *log){
	int i,fd;
	struct stat st;
	void *addr;
	for(i=0; i<2; i++){
		if (stat(log->m[i].file_name, &st) == -1) {
			first_run=1;
    	}	

		fd = open (log->m[i].file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		lseek (fd, log->log_size, SEEK_SET);
		write (fd, "", 1);
		lseek (fd, 0, SEEK_SET);
		addr = mmap (NULL,log->log_size, PROT_WRITE, MAP_SHARED, fd, 0);
		if(addr == MAP_FAILED){
			printf("Runtime Error: error while memory mapping the file\n");
			exit(1);
		}
		if(isDebugEnabled()){
			printf("mapped the file: %s \n", log->m[i].file_name);
		}
		log->m[i].map_file_ptr = addr;
		log->m[i].head = log->m[i].map_file_ptr;
		log->m[i].meta =(checkpoint_t *)((headmeta_t *)log->m[i].map_file_ptr)+1;
		close (fd);
		

	}
	
	if(first_run){
		if(isDebugEnabled()){
			printf("first run of the library. Initializing the mapped files..\n");
		}
		for(i=0;i<2;i++){
			headmeta_t head;
			head.offset = -1;
            head.current_version = -1;
			gettimeofday(&(head.timestamp),NULL);
			memcpy(log->m[i].head,&head,sizeof(headmeta_t));
			//TODO: introduce a magic value to check atomicity
		}
		log->current = &log->m[0];
		if(isDebugEnabled()){
			headmeta_t *h1 = log->m[0].head;
			headmeta_t *h2 = log->m[1].head;
			printf("init_map_files:timestamp of log[0] %ld.%06ld\n", h1->timestamp.tv_sec, h1->timestamp.tv_usec);
			printf("init_map_files:timestamp of log[1] %ld.%06ld\n", h2->timestamp.tv_sec, h2->timestamp.tv_usec);

		}
        checkpoint_version = 0;
	}else{
		if(isDebugEnabled()){
			printf("restart run. Most recent map file set as current.\n");
		}
		log->current = get_latest_mapfile(log);
        //restoring latest stable checkpoint of the system
        checkpoint_version = log->current->head->current_version;

    }
}

/* return the log with latest timestamp out of two log files */
static memmap_t *get_latest_mapfile(log_t *log){
    //first check the time stamps of the head values
    headmeta_t *h1 = log->m[0].head;
    headmeta_t *h2 = log->m[1].head;
	if(isDebugEnabled()){
			headmeta_t *h1 = log->m[0].head;
			headmeta_t *h2 = log->m[1].head;
			printf("init_map_files:timestamp of log[0] %ld.%06ld\n", h1->timestamp.tv_sec, h1->timestamp.tv_usec);
			printf("offset of log[0] %ld , version of log[0] %ld \n", h1->offset, h1->current_version);
			printf("init_map_files:timestamp of log[1] %ld.%06ld\n", h2->timestamp.tv_sec, h2->timestamp.tv_usec);
			printf("log offset of log[1] %ld , version of log[1] %ld\n", h2->offset , h2->current_version);

		}

    if(timercmp(&(h1->timestamp),&(h2->timestamp),<) && (h1->current_version !=-1)){
        return &log->m[0];
    }else if(h2->current_version != -1){
        return &log->m[1];
    }else{
        printf("Runtime Error: wrong program execution path...\n");
		printf("timestamp of log[0] %ld.%06ld\n", h1->timestamp.tv_sec, h1->timestamp.tv_usec);
        printf("offset of log[0] %ld , version of log[0] %ld \n", h1->offset, h1->current_version);
		printf("timestamp of log[1] %ld.%06ld\n", h2->timestamp.tv_sec, h2->timestamp.tv_usec);
        printf("log offset of log[1] %ld , version of log[1] %ld\n", h2->offset , h2->current_version);
        assert(0);
    }
    return NULL;
}

static void checkpoint(log_t *log, char *var_name, int process_id, int version, size_t size, void *data,int wbw){
    checkpoint_t chkpt;
    void *start_addr;
	checkpoint_t *cbptr = log->current->meta;
	offset_t head_offset = log->current->head->offset;
	
    strncpy(chkpt.var_name,var_name,VAR_SIZE-1);
    chkpt.var_name[VAR_SIZE-1] = '\0'; // null terminating
	if(isDebugEnabled()){
		//printf("checkpoint var name : %s\n",chkpt.var_name);
	}
    chkpt.process_id = process_id;
    chkpt.version = version;
    chkpt.data_size = size;
    if(head_offset != -1 ){
        checkpoint_t *last_meta = get_meta(cbptr, head_offset);
        start_addr = get_start_addr(cbptr, last_meta);
        chkpt.prv_offset = head_offset;
        chkpt.offset = last_meta->offset + sizeof(checkpoint_t)+last_meta->data_size;
    }else{
        start_addr = cbptr;
        chkpt.prv_offset = -1;
        chkpt.offset = 0;
    }
    checkpoint1(log,start_addr, &chkpt,data,wbw);
    return;
}

static void checkpoint1(log_t *log, void *start_addr, checkpoint_t *chkpt, void *data,int wbw){
    memcpy(start_addr,chkpt,sizeof(checkpoint_t));
    void *data_offset = ((char *)start_addr)+sizeof(checkpoint_t);
    nvmmemcpy_write(data_offset,data,chkpt->data_size,wbw);
    log->current->head->offset = chkpt->offset;
    return;
}


/*utility methods*/

static void *get_start_addr(void *base_addr,checkpoint_t *last_meta){
    size_t tot_offset = last_meta->offset + sizeof(checkpoint_t) + last_meta->data_size;
    char *next_start_addr = (char *)base_addr+tot_offset;
    return (void *)next_start_addr;
}

static checkpoint_t *get_meta(void *base_addr,size_t offset){
    checkpoint_t *ptr = (checkpoint_t *)(((char *)base_addr) + offset);
    return ptr;
}

static int is_remaining_space_enough(log_t *log, var_t *list){
    //TODO: This does not take checkpoint location in to account
	long tot_size=0;
	var_t *s;
	for (s = list; s != NULL; s = s->hh.next){
				tot_size += (sizeof(checkpoint_t)+ s->size);
	}	
	if(tot_size > (log->log_size - sizeof(headmeta_t))){
		printf("allocated buffer is not sufficient for program exec\n");
		assert(0);
	}
	checkpoint_t *cp = get_meta(log->current->meta, log->current->head->offset);
	size_t remaing_size = log->log_size - (sizeof(headmeta_t) + log->current->head->offset+1 + sizeof(checkpoint_t) + cp->data_size ); //adding 1 since offset can be -1
	if(isDebugEnabled()){
		printf("[%d] offset : remaining : checkpoint  sizes - %ld : %ld : %ld \n",lib_process_id,log->current->head->offset,remaing_size,tot_size);
	}
	return (remaing_size > tot_size); 
}


/*
 * find out the current log has enough space to accomadate the
 * destage data.
 */
static int is_remaining_space_enough2(log_t *log, dcheckpoint_map_entry_t *map){
    long tot_size=0;

    dcheckpoint_map_entry_t *s;
    for(s=map;s!=NULL;s=s->hh.next){
        assert(s != NULL);
        //log_info("[%d] variable name : %s", lib_process_id,s->var_name);
        //log_info("[%d] variable size : %ld",lib_process_id,s->size);
        tot_size += (sizeof(checkpoint_t) + s->size);
    }
    checkpoint_t *cp = get_meta(log->current->meta, log->current->head->offset);
    size_t remaing_size = log->log_size - (sizeof(headmeta_t) + log->current->head->offset+1 + sizeof(checkpoint_t) + cp->data_size ); //adding 1 since offset can be -1
    if(isDebugEnabled()){
        printf("[%d] offset : remaining : checkpoint  sizes - %ld : %ld : %ld \n",lib_process_id,log->current->head->offset,remaing_size,tot_size);
    }
    return (remaing_size > tot_size);
}
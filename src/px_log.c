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
#include "px_checkpoint.h"
#include "px_util.h"

#define FILE_PATH_ONE "/mnt/ramdisk/mmap.file.one"
#define FILE_PATH_TWO "/mnt/ramdisk/mmap.file.two"
#define MICROSEC 1000000

//global variables
int first_run=0;

int is_chkpoint_present(log_t *log);
static void init_mmap_files(log_t *log);
static memmap_t *get_latest_mapfile(log_t *log);
static void checkpoint(log_t *log, char *var_name, int process_id, int version, size_t size, void *data);
static void checkpoint1(log_t *log,void *start_addr, checkpoint_t *chkpt, void *data);
static checkpoint_t *get_meta(void *base_addr,size_t offset);
static void *get_start_addr(void *base_addr,checkpoint_t *last_meta);



void log_init(log_t *log , long log_size, int process_id){
	if(isDebugEnabled()){
		printf("initializing the structures... %d \n", process_id);
	}
    snprintf(log->m[0].file_name, sizeof(log->m[0].file_name), "%s%d",FILE_PATH_ONE,process_id);
    snprintf(log->m[1].file_name, sizeof(log->m[1].file_name),"%s%d",FILE_PATH_TWO,process_id);
    init_mmap_files(log);
}

int log_write(log_t *log, listhead_t *lhead, int process_id){
	entry_t *np;
    for (np = lhead->lh_first; np != NULL; np = np->entries.le_next){
        if(np->process_id == process_id){ 
            checkpoint(log, np->var_name, np->process_id, np->version, np->size, np->ptr);
        }
    }	

	return 1;
}

checkpoint_t *log_read(log_t *log, char *var_name, int process_id){
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
			printf("comparing values  process ids (%d, %d) - (%s, %s)\n",ptr->process_id, process_id,ptr->var_name,var_name);
		}
		if((ptr->process_id == process_id) && (str_cmp=strncmp(ptr->var_name,var_name,10))==0){ // last char is null terminator
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
	for(i=0; i<2; i++){
		if (stat(log->m[i].file_name, &st) == -1) {
			first_run=1;
    	}	

		fd = open (log->m[i].file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		lseek (fd, log->log_size, SEEK_SET);
		write (fd, "", 1);
		lseek (fd, 0, SEEK_SET);
		log->m[i].map_file_ptr = mmap (0,log->log_size, PROT_WRITE, MAP_SHARED, fd, 0);
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
			gettimeofday(&(head.timestamp),NULL);
			memcpy(log->m[i].head,&head,sizeof(headmeta_t));
			//TODO: introduce a magic value to check atomicity
		}
		log->current = &log->m[0];
	}else{
		if(isDebugEnabled()){
			printf("restart run. Most recent map file set as current.\n");
		}
		log->current = get_latest_mapfile(log);
	}
}

/* return the log with latest timestamp out of two log files */
static memmap_t *get_latest_mapfile(log_t *log){
    //first check the time stamps of the head values
    headmeta_t *h1 = log->m[0].head;
    headmeta_t *h2 = log->m[1].head;
    struct timeval result;
    if(!timeval_subtract(&result, &(h1->timestamp),&(h2->timestamp)) && (h1->offset !=-1)){
        return &log->m[0];
    }else if(h2->offset != -1){
        return &log->m[1];
    }else{
        printf("Runtime Error: wrong program execution path...\n");
        assert(0);
    }
}

static void checkpoint(log_t *log, char *var_name, int process_id, int version, size_t size, void *data){
    checkpoint_t chkpt;
    void *start_addr;
	checkpoint_t *cbptr = log->current->meta;
	offset_t head_offset = log->current->head->offset;
	
    strncpy(chkpt.var_name,var_name,VAR_SIZE-1);
    chkpt.var_name[VAR_SIZE-1] = '\0'; // null terminating
    printf("checkpoint var name : %s\n",chkpt.var_name);
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
    checkpoint1(log,start_addr, &chkpt,data);
    return;
}

static void checkpoint1(log_t *log, void *start_addr, checkpoint_t *chkpt, void *data){
    memcpy(start_addr,chkpt,sizeof(checkpoint_t));
    void *data_offset = ((char *)start_addr)+sizeof(checkpoint_t);
    memcpy_write(data_offset,data,chkpt->data_size);
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

/*static void *get_addr(void *base_addr, size_t offset){
    char *temp = ((char *)base_addr)+offset;
    return temp;
}

static int get_new_offset(offset_t offset, size_t data_size){
    int temp = offset + sizeof(checkpoint_t) + data_size;
    return temp;
}*/

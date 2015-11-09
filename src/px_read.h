#ifndef __PX_READ_H
#define __PX_READ_H

#include <pthread.h>

#include "uthash.h"
#include "px_checkpoint.h"
#include "px_log.h"

typedef struct pagemap_t_ {
    void *pageptr;                    /* key */
    void *nvpageptr;            
	void **remote_ptr;
    offset_t size;
	offset_t paligned_size;
    UT_hash_handle hh;         /* makes this structure hashable */
	int copied;
    int started_tracking;
    struct timeval start_timestamp; /* start and end time stamp to monitor access patterns */
    struct timeval end_timestamp;
    struct timeval earlycopy_timestamp;
	char varname[20];

} pagemap_t;


void *copy_read(log_t *, char *var_name,int process_id,long version);
void *chunk_read(log_t *, char *var_name,int process_id);
void *pc_read(log_t *log, char *var_name, int process_id);
void *page_aligned_copy_read(log_t *log, char *var_name,int process_id);


void remote_copy_read(log_t *, char *var_name,int process_id,entry_t *entry);
void remote_chunk_read(log_t *, char *var_name,int process_id,entry_t *entry);
void remote_pc_read(log_t *, char *var_name,int process_id,entry_t *entry);
#endif

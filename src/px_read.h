#ifndef __PX_READ_H
#define __PX_READ_H

#include <pthread.h>

#include "uthash.h"
#include "px_checkpoint.h"
#include "px_log.h"

typedef struct pagemap_t_ {
    void *pageptr;                    /* key */
    void *nvpageptr;            
    offset_t size;
	offset_t paligned_size;
    UT_hash_handle hh;         /* makes this structure hashable */
	int copied;
} pagemap_t;


void *copy_read(log_t *, char *var_name,int process_id);
void *chunk_read(log_t *, char *var_name,int process_id);
void *pc_read(log_t *log, char *var_name, int process_id);
void *page_aligned_copy_read(log_t *log, char *var_name,int process_id);


#endif

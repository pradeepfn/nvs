#ifndef __PX_READ_H
#define __PX_READ_H

#include "uthash.h"
#include "px_checkpoint.h"

typedef struct pagemap_t_ {
    void *pageptr;                    /* key */
    void *nvpageptr;                    /* key */
    offset_t size;
	offset_t paligned_size;
    UT_hash_handle hh;         /* makes this structure hashable */
} pagemap_t;


void *copy_read(log_t *, char *var_name,int process_id);
void *fault_read(log_t *, char *var_name,int process_id);


#endif

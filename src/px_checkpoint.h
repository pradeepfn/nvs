#ifndef __PX_CHECKPOINT_H
#define __PX_CHECKPOINT_H

#include <sys/queue.h>
#include <signal.h>
#include "uthash.h"



typedef long offset_t;

typedef enum
{
    DRAM_CHECKPOINT,
    NVRAM_CHECKPOINT
}checkpoint_type;

typedef struct var_t_ {
    void *ptr;       /* memory address of the variable*/
    void *nvptr;   /*this get used in the pre-copy in the restart execution*/
    void **remote_ptr; /*pointer grid of memory group */
    void *local_remote_ptr;  /* local pointer out of memory grid */
    offset_t size;
    offset_t paligned_size;
    UT_hash_handle hh;         /* makes this structure hashable */
    int early_copied;
    int started_tracking;
    int process_id;
    checkpoint_type type;
    //struct timeval start_timestamp; /* start and end time stamp to monitor access patterns */
    struct timeval end_timestamp; /*last access time of the variable*/
    struct timeval earlycopy_time_offset; /* time offset since checkpoint, before starting early copy */
    char varname[20];  /* key */

} var_t;


//struct to distribute early copy offset

typedef struct timeoffset_t_{
    char varname[20];
    ulong seconds;
    ulong micro;
} timeoffset_t;

#endif

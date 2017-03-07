#ifndef __PX_LOG_H
#define __PX_LOG_H


#include "px_types.h"

/*
 * An element in the ring buffer, the offsets points to data locations in linear log
 */
typedef struct checkpoint_t_{
    char var_name[20];
    int process_id;
    ulong version;
    ulong size;
    unsigned char hash[MD5_LENGTH]; // 64 bit hash value
    long start_offset;  // start offset in linear log
    long end_offset;   // end offset in linear log
}checkpoint_t;

typedef struct headmeta_t_{
    pthread_mutex_t plock;
    ulong head; // head index of the ringbuffer
    ulong tail; // tail index
    ulong current_version; // new atomic flag that uses checkpoint version
    uint nelements; // number of buffer slots in ring buffer
    ulong log_size; // size of the linear log
	ushort log_initialized; // check flag before plock init. 
}headmeta_t;



/*
 -----------------
| Head  |Element|
|-------|-------|-

 * soft state structure
 */

typedef struct ringbuffer_t_{
	char file_name[256];
    headmeta_t *head;
    checkpoint_t *elem_start_ptr;
    //ulong log_head; // head offset of the linear log, caching purposes
    //ulong log_tail; // tail offset of the linear log, caching purposes
}ringbuffer_t;

/* log data strucuture that get data stored in */
typedef struct logdata_t_{
    void *start_ptr;
    char file_name[256];
    ulong log_size;
}logdata_t;

/*we use this structure to identify valid
 * data in the linear log. If this is not set, the memcpy
 * failed in the middle of the copy
*/
typedef struct preamble_t_{
    long value;
}preamble_t;



/*
 * log structure exposed by the API.
 */
typedef struct log_t_{
	ringbuffer_t ring_buffer;
    logdata_t data_log;
    rcontext_t *runtime_context;
    pthread_mutex_t *plock;
}log_t;

int create_shm(char *ishm_name, char *dshm_name,ulong log_size);
int log_init(log_t *log, int proc_id);
int log_write(log_t *,var_t *,long);
checkpoint_t *log_read(log_t *, char *, int , long);
int log_commitv(log_t *log,ulong version);
int is_chkpoint_present(log_t *log);
void* log_ptr(log_t *log,ulong offset);
int log_isfull(log_t *log);
int log_isempty(log_t *log);
#endif


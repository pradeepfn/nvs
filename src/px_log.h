#ifndef __PX_LOG_H
#define __PX_LOG_H

#include "px_checkpoint.h"
#include "px_dlog.h"

typedef long offset_t;

typedef struct checkpoint_t_{
    char var_name[20];
    int process_id;
    long version;
    offset_t data_size;
    offset_t prv_offset;
    offset_t offset;
}checkpoint_t;

typedef struct headmeta_t_{
    offset_t offset; // record the offest, and i used it as atomic checkpoint flag
    long current_version; // new atomic flag that uses checkpoint version
    long online_version; // online stable checkpoint version achieved when we double checkpoint data.
    struct timeval timestamp;
}headmeta_t;

typedef struct memmap_t_{
    void *map_file_ptr;
	char file_name[256];
    headmeta_t *head;
    checkpoint_t *meta;
}memmap_t;

typedef struct log_t_{
	memmap_t m[2];
   // pthread_mutex_t mtx; // only one thread can write to log file
	memmap_t *current;
	offset_t log_size;
	offset_t offset;

}log_t;


void log_init(log_t *, long, int);
int log_write(log_t *,listhead_t *,int,long);
int log_write_var(log_t *, entry_t *, long);
checkpoint_t *log_read(log_t *, char *, int , long);
int is_chkpoint_present(log_t *log);

//int remote_data_log_write(log_t *,listhead_t *,int);
int destage_data_log_write(log_t *log,dcheckpoint_map_entry_t *map,int process_id);
#endif


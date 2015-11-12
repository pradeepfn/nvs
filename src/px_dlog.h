#ifndef PHOENIX_PX_DLOG_H
#define PHOENIX_PX_DLOG_H

#include "uthash.h"
#include "px_checkpoint.h"


typedef enum { // type we used for seperating double in memory remote and local checkpoint versions
    DOUBLE_IN_MEMORY_LOCAL,
    DOUBLE_IN_MEMORY_REMOTE
} dim_type;

typedef struct dcheckpoint_map_entry_t_{
    char var_name[20];    /* key */
    int process_id;
    long version;
    void *data_ptr;
    size_t size;
    UT_hash_handle hh;        /* makes this structure hashtable */

}dcheckpoint_map_entry_t;

typedef struct dlog_t_{
    dcheckpoint_map_entry_t *map[2];

    long dlog_checkpoint_version; //current version no of the checkpoint
    int is_dlog_valid; // used for atomic udate the buffer

    long dlog_remote_checkpoint_version; // track version and validity of remote checkpoint
    int is_dlog_remote_valid;

}dlog_t;





void dlog_init(dlog_t *dlog);
int dlog_remote_write(dlog_t *dlog, var_t *list,int process_id,long version);
int dlog_local_write(dlog_t *dlog, var_t *list,int process_id, long version);
dcheckpoint_map_entry_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, long version, checkpoint_type type);
#endif //PHOENIX_PX_DLOG_H



//
// Created by pradeep on 9/17/15.
//

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
    int version;
    void *data_ptr;
    size_t size;
    UT_hash_handle hh;        /* makes this structure hashtable */

}dcheckpoint_map_entry_t;

typedef struct dlog_t_{
    dcheckpoint_map_entry_t *map[2];
}dlog_t;


void dlog_init(dlog_t *dlog);
int dlog_remote_write(dlog_t *dlog, listhead_t *lhead,int process_id);
int dlog_local_write(dlog_t *dlog, listhead_t *lhead,int process_id);
dcheckpoint_map_entry_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, checkpoint_type type);
#endif //PHOENIX_PX_DLOG_H



#ifndef PHOENIX_PX_DLOG_H
#define PHOENIX_PX_DLOG_H

#include "uthash.h"
#include "px_types.h"


typedef enum { // type we used for seperating double in memory remote and local checkpoint versions
    DOUBLE_IN_MEMORY_LOCAL,
    DOUBLE_IN_MEMORY_REMOTE
} dim_type;

typedef struct dlog_t_{
    var_t *map[2];

    long dlog_checkpoint_version; //current version no of the checkpoint
    int is_dlog_valid; // used for atomic udate the buffer
    rcontext_t *runtime_context;

    long dlog_remote_checkpoint_version; // track version and validity of remote checkpoint
    int is_dlog_remote_valid;

}dlog_t;





void dlog_init(dlog_t *dlog);
int dlog_remote_write(dlog_t *dlog, var_t *list,int process_id,long version);
int dlog_local_write(dlog_t *dlog, var_t *list,int process_id, long version);
var_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, long version, checkpoint_type type);
#endif //PHOENIX_PX_DLOG_H



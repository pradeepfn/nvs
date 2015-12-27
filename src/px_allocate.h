//
// Created by pradeep on 9/29/15.
//


#ifndef PHOENIX_PX_ALLOCATE_H
#define PHOENIX_PX_ALLOCATE_H

#include "px_read.h"


typedef struct allocate_t_{
    var_t *pagemap;
    long page_size;
} allocate_t;

var_t *px_alighned_allocate(size_t size ,int process_id, char *varname);
void stop_page_tracking();
void start_page_tracking();
void reset_trackers();
void flush_access_times();
void decide_checkpoint_split(rcontext_t *runtime_context, var_t *list,long long freemem);
void calc_early_copy_times(rcontext_t *runtime_context);
void broadcast_page_tracking(rcontext_t *runtime_context);

#endif //PHOENIX_PX_ALLOCATE_H

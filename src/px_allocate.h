//
// Created by pradeep on 9/29/15.
//

#include "px_checkpoint.h"
#include "px_read.h"

#ifndef PHOENIX_PX_ALLOCATE_H
#define PHOENIX_PX_ALLOCATE_H


typedef struct allocate_t_{
    pagemap_t *pagemap;
    long page_size;
} allocate_t;

void *px_alighned_allocate(size_t size , char *varname);
void stop_page_tracking();
void flush_access_times();
void decide_checkpoint_split(listhead_t *head,long long freemem);

#endif //PHOENIX_PX_ALLOCATE_H

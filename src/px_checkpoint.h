#ifndef __PX_CHECKPOINT_H
#define __PX_CHECKPOINT_H

#include <sys/queue.h>

#define VAR_SIZE 20

LIST_HEAD(listhead, entry);
struct entry {
    void *ptr;
    size_t size;
    int id;
    char var_name[VAR_SIZE];
    int process_id;
    int version;
    LIST_ENTRY(entry) entries;
};
typedef struct entry entry_t;
typedef struct listhead listhead_t;
#endif

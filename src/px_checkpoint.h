#ifndef __PX_CHECKPOINT_H
#define __PX_CHECKPOINT_H

#include <sys/queue.h>
#include <signal.h>

#define VAR_SIZE 20

typedef enum
{
    DRAM_CHECKPOINT,
    NVRAM_CHECKPOINT
}checkpoint_type;

LIST_HEAD(listhead, entry);
struct entry {
    void *ptr;
    size_t size;
    int id;
    char var_name[VAR_SIZE];
    int process_id;
    int early_copied;
    checkpoint_type type;
    LIST_ENTRY(entry) entries;
    /*Remote checkpoint specific members*/
    void **rmt_ptr;   /*pointer grid of memory group */
	void *local_ptr;  /* local pointer out of memory grid */
};

LIST_HEAD(tlisthead, thread_t_);
struct thread_t_{
	pthread_t pthreadid;
	volatile sig_atomic_t flag;
    LIST_ENTRY(thread_t_) entries;
}; 

typedef struct tcontext_t_{
	void *addr;
	int chunk_size;
}tcontext_t;

typedef struct entry entry_t;
typedef struct listhead listhead_t;
typedef struct thread_t_ thread_t;
typedef struct tlisthead tlisthead_t;
#endif

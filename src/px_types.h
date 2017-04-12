
#ifndef PHOENIX_PX_TYPES_H
#define PHOENIX_PX_TYPES_H

#include <sys/time.h>
#include <sys/types.h>
#include <semaphore.h>

#include "uthash.h"
#include "px_constants.h"
#include "phoenix.h"


struct log_t_;
struct dlog_t_;
struct var_t_;

/* holds the static config of the runtime
 * This dat-type models the config file */
typedef struct ccontext_t_{

    long log_size;
    int chunk_size;
    int copy_strategy;
    int nvram_wbw;
    int restart_run;
    int remote_checkpoint;
    int remote_restart;
    char pfile_location[32];
    int buddy_offset;
    int split_ratio;
    ulong free_memory;
    int threshold_size;
    long max_checkpoints;
    int early_copy_enabled;
    int helper_cores[5];
    int helper_core_size;
    int cr_type;
    ulong ec_offset_add;
	char lckfile[50];

} ccontext_t;


/* holds the runtime context of the lib */
typedef struct rcontext_t_{

    ccontext_t *config_context;
    struct timeval lchk_start_time; // last checkpoint time
    struct timeval lchk_end_time; // end time of las checkpoint
    struct timeval lchk_iteration_time; // last iteration time
    ulong checkpoint_version;
    //threadpool_t *thread_pool;
    long free_memory;
    ulong checkpoint_iteration;
    struct timeval px_start_time;
    ulong nvram_checkpoint_size;
    ulong local_dram_checkpoint_size;
    ulong remote_dram_checkpoint_size;
    int process_id;
    int nproc;
    int ec_finished; /* flag variable to check if early copy thread finished*/
    int ec_abort; /* flag variable to signal ec thread - stop early copy */
    int ec_start; /* notify to start early copy. set by destage or main thread */
    //sem_t ec_sem; /* signaling semaphore to co-ordinate between mainthread and early copy thread*/
    pthread_mutex_t mtx; /* mutex to protect shared variables between, main thread, destage thread and ec thread */
    pthread_cond_t cond; /* use to singaling between ec and main threads */
    pthread_cond_t cond_ec_start; /* signal between destage and ec threads */
    struct log_t_ *nvlog;
    struct dlog_t_ *dlog;
    struct var_t_ **varmap; /* pointer to varmap pointer */

}rcontext_t;



typedef ulong offset_t;

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
    volatile int early_copied; // get accesed by signal handler
    int started_tracking;
    int process_id;
    checkpoint_type type;
    //struct timeval start_timestamp; /* start and end time stamp to monitor access patterns */
    struct timeval end_timestamp; /*last access time of the variable*/
    struct timeval earlycopy_time_offset; /* time offset since checkpoint, before starting early copy */
    ulong version; // use in dlog hash structure.
    unsigned char hash[MD5_LENGTH]; // used for md5 digest store
	long dv_size; // number of int elements in the dedup vector
	int *dedup_vector; //vector for tracking modified memory pages
	float mod_average;
    char key1[20];  /* key */
	px_obj cobj; //hack to read snapshots

} var_t;


//struct to distribute early copy offset

typedef struct timeoffset_t_{
    char varname[20];
    ulong seconds;
    ulong micro;
} timeoffset_t;


typedef struct destage_t_{
    struct dlog_t_ *dlog;
    struct log_t_ *nvlog;
    int process_id;
    long checkpoint_version;
}destage_t;


typedef struct stat_t_{
	

}stat_t;


#endif //PHOENIX_PX_TYPES_H

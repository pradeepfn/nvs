#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define RING_BUFFER_SLOTS 300
#define KEY_LENGTH 20
#define PAGE_SIZE 4096
#define CONFIG_FILE_NAME "phoenix.config"

//config file params
#define NVM_SIZE "nvm.size"
#define DEBUG_ENABLE "debug.enable"
#define PFILE_LOCATION "pfile.location"
#define LCKFILE_LOCATION "lckfile.location"
#define NVRAM_WBW "nvram.wbw"
#define MAX_CHECKPOINTS "max.checkpoints"
#define MD5_LENGTH 16


#define CHUNK_SIZE "chunk.size"
#define COPY_STRATEGY "copy.strategy"
#define NVRAM_EARLY_COPY_WBW "nvram.ec.wbw"
#define RSTART "rstart"
#define REMOTE_CHECKPOINT_ENABLE "rmt.chkpt.enable"
#define REMOTE_RESTART_ENABLE "rmt.rstart.enable"
#define BUDDY_OFFSET "buddy.offset"
#define SPLIT_RATIO "split.ratio"
#define CR_TYPE "cr.type"
#define FREE_MEMORY "free.memory"
#define THRESHOLD_SIZE "threshold.size"
#define EARLY_COPY_ENABLED "early.copy.enabled"
#define EARLY_COPY_OFFSET "early.copy.offset"  // (microseconds) when early copy invalidated. we increase the early copy time by this amount
#define HELPER_CORES "helper.cores"
#define NAIVE_COPY 1



#define mb()    asm volatile("mfence" : : : "memory")

#endif

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define FAULT_COPY_WAIT		0
#define NO_WAIT				1
#define TRADITIONAL_CR 0
#define ONLINE_CR 1
#define RING_BUFFER_SLOTS 120
#define VAR_SIZE 20
#define MAGIC_VALUE 342312

#define CONFIG_FILE_NAME "phoenix.config"

//config file params
#define NVM_SIZE "nvm.size"
#define CHUNK_SIZE "chunk.size"
#define COPY_STRATEGY "copy.strategy"
#define DEBUG_ENABLE "debug.enable"
#define PFILE_LOCATION "pfile.location"
#define NVRAM_WBW "nvram.wbw"
#define NVRAM_EARLY_COPY_WBW "nvram.ec.wbw"
#define RSTART "rstart"
#define REMOTE_CHECKPOINT_ENABLE "rmt.chkpt.enable"
#define REMOTE_RESTART_ENABLE "rmt.rstart.enable"
#define BUDDY_OFFSET "buddy.offset"
#define SPLIT_RATIO "split.ratio"
#define CR_TYPE "cr.type"
#define FREE_MEMORY "free.memory"
#define THRESHOLD_SIZE "threshold.size"
#define MAX_CHECKPOINTS "max.checkpoints"
#define EARLY_COPY_ENABLED "early.copy.enabled"
#define EARLY_COPY_OFFSET "early.copy.offset"  // (microseconds) when early copy invalidated. we increase the early copy time by this amount
#define HELPER_CORES "helper.cores"
#define NAIVE_COPY 1


#define mb()    asm volatile("mfence" : : : "memory")

#endif

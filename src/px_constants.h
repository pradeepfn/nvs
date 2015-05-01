#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define IDLE				0
#define FAULT_COPY			1
#define THREAD_COPY			2
#define FAULT_COPY_WAIT		3
#define FAULT_COPY_YIELD	4
#define NO_WAIT				5

#define mb()    asm volatile("mfence" : : : "memory")

#endif

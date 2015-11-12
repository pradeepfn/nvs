#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define FAULT_COPY_WAIT		0
#define NO_WAIT				1
#define TRADITIONAL_CR 0
#define ONLINE_CR 1


#define mb()    asm volatile("mfence" : : : "memory")

#endif

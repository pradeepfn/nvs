//
// Created by pradeep on 10/2/17.
//

#ifndef MMAPBENCH_UTILS_H
#define MMAPBENCH_UTILS_H

#include <stdint.h>
#include <sched.h>
#include <stdio.h>
#include <x86intrin.h>

#define FLUSH_ALIGN ((uintptr_t)64)

/* some util functions -- some of implementations were copied from whisper code repository*/

typedef uint64_t mem_word_t;


static inline void asm_clflush(mem_word_t *addr)
{
    __asm__ __volatile__ ("clflush %0" : : "m"(*addr));
}

static inline void asm_mfence(void)
{
    __asm__ __volatile__ ("mfence");
}


static inline void asm_sfence(void)
{
    __asm__ __volatile__ ("sfence");
}

static void
flush_clflush(const void *addr, size_t len)
{

    uintptr_t uptr;

    /*
     * Loop through cache-line-size (typically 64B) aligned chunks
     * covering the given range.
     */
    for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
         uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN)
        _mm_clflush((char *)uptr);
}



/*static void
flush_clflushopt(const void *addr, size_t len)
{

    uintptr_t uptr;

    *//*
     * Loop through cache-line-size (typically 64B) aligned chunks
     * covering the given range.
     *//*
    for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
         uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
        _mm_clflushopt((char *)uptr);
    }
}*/






#endif //MMAPBENCH_UTILS_H

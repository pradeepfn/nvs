//
// Created by pradeep on 10/2/17.
//

#ifndef MMAPBENCH_UTILS_H
#define MMAPBENCH_UTILS_H

#include <stdint.h>
#include <sched.h>
#include <stdio.h>
#include <x86intrin.h>
#include <assert.h>
#include "debug.h"

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



/* Copies memory from src to dst, using SSE 4.1's MOVNTDQA to accelerate write performance
 */
void
 streaming_memcpy(void *dst, void *src, size_t len)
{
    char *d = dst;
    char *s = src;

    /* If dst and src are not co-aligned, its an error */
    if (((uintptr_t)d & 15) || ((uintptr_t)s & 15)) {
        log_err("dst or source does not have 16 byte starting boundry");
        exit(-1);
    }

    if (len%16){
        log_err("no multiples of sixteen bytes");
        exit(-1);
    }

    while (len >= 64) {
        __m128i *dst_cacheline = (__m128i *)d;
        __m128i *src_cacheline = (__m128i *)s;

        _mm_store_si128(dst_cacheline + 0, src_cacheline[0]);
        _mm_store_si128(dst_cacheline + 1, src_cacheline[1]);
        _mm_store_si128(dst_cacheline + 2, src_cacheline[2]);
        _mm_store_si128(dst_cacheline + 3, src_cacheline[3]);

        d += 64;
        s += 64;
        len -= 64;
    }

    assert(len == 0);
}




#endif //MMAPBENCH_UTILS_H

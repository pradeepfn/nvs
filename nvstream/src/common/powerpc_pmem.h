//
// Created by pradeep on 10/7/18.
//

#ifndef NVSTREAM_POWERPC_PMEM_H
#define NVSTREAM_POWERPC_PMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#define forceinline inline __attribute__((always_inline))

#define FLUSH_ALIGN ((uintptr_t)64)
typedef uintptr_t memword_t;

#define sfence() asm volatile("sync")
#define mfence() asm volatile("sync")


/*
 * usage learn: https://android.googlesource.com/platform/prebuilts/gcc/darwin-x86/host/i686-apple-darwin-4.2.1/+/android-4.3_r1.1/include/gcc/darwin/4.2/ppc_intrinsics.h
 *
 */

static forceinline void _clflush(memword_t *addr)
{
    __asm__ ("dcbf %0, %1" : /*no result*/ : "b%" (0), "r" (addr) : "memory");
}

static forceinline void
flush_clflush(const void *addr, size_t len)
{

    uintptr_t uptr;

    /*
     * Loop through cache-line-size (typically 64B) aligned chunks
     * covering the given range.
     */
    for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
         uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
        __asm__ ("dcbf %0, %1" : /*no result*/ : "b%" (0), "r" (addr) : "memory");
    }
    //full memory fence as PPC has relaxed memory consistency
}

#define nvs_memcpy(dst, src, n)              \
	((__builtin_constant_p(n)) ?          \
	memcpy((dst), (src), (n)) :          \
	powerpc_memcpy_func((dst), (src), (n)))

static forceinline void *
powerpc_memcpy_func(void *dst, const void *src, size_t n) __attribute__((always_inline));

static forceinline void *
powerpc_memcpy_func(void *dst, const void *src, size_t n)
{
    memcpy(dst,src,n);
}

#define sse_memcpy(dst, src, n)

#ifdef __cplusplus
}
#endif
#endif //NVSTREAM_POWERPC_PMEM_H

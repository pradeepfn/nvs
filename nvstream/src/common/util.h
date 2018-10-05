#ifndef NVSTREAM_UTIL_H
#define NVSTREAM_UTIL_H

#include <sched.h>
#include <cstdint>
#include <cstdio>
#include <signal.h>
#include <sys/mman.h>

#include "nvs/log.h"

namespace nvs {

    static void install_sighandler(void (*sighandler)(int, siginfo_t *, void *), struct sigaction *old_sa) {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = sighandler;
        if (sigaction(SIGSEGV, &sa,old_sa ) == -1) {
            LOG(SeverityLevel::error) << "sigaction";
            exit(1);
        }
    }


    static void install_old_handler(struct sigaction *old_sa) {
        struct sigaction temp;
        LOG(SeverityLevel::debug) << "old signal handler installed";
        if (sigaction(SIGSEGV, old_sa, &temp) == -1) {
            LOG(SeverityLevel::error) << "sigaction";
            exit(1);
        }
    }

    static void call_oldhandler(int signo,struct sigaction *old_sa) {
        LOG(SeverityLevel::debug) << "old signal handler called";
        (*old_sa->sa_handler)(signo);
    }

    static void enable_protection(void *ptr, size_t aligned_size) {
        if (mprotect(ptr, aligned_size, PROT_NONE) == -1) {
            LOG(SeverityLevel::error) <<"mprotect";
        }
    }

    static void enable_write_protection(void *ptr, size_t aligned_size) {
        if (mprotect(ptr, aligned_size, PROT_READ) == -1) {
            LOG(SeverityLevel::error) <<"mprotect";
        }
    }


/*
* Disable the protection of the whole aligned memory block related to each variable access
* return : size of memory
*/
static long disable_protection(void *page_start_addr, size_t aligned_size) {
        if (mprotect(page_start_addr, aligned_size, PROT_WRITE) == -1) {
            LOG(SeverityLevel::error) <<"mprotect";
        }
        return aligned_size;
    }


}


static void gdb_attach()
{
	    int i = 0;
	    char hostname[256];
	    gethostname(hostname, sizeof(hostname));
	    printf("PID %d on %s ready for attach\n", getpid(), hostname);
	    fflush(stdout);
	    while (0 == i)
	        sleep(5);
}

static int
affinity_set(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    return sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

static uint64_t
get_cpu_freq(void)
{
    FILE *fd;
    uint64_t freq = 0;
    float freqf = 0;
    char *line = NULL;
    size_t len = 0;

    fd = fopen("/proc/cpuinfo", "r");
    if (!fd) {
        fprintf(stderr, "failed to get cpu frequecy\n");
        perror(NULL);
        return freq;
    }

    while (getline(&line, &len, fd) != EOF) {
        if (sscanf(line, "cpu MHz\t: %f", &freqf) == 1) {
            freqf = freqf * 1000000UL;
            freq = (uint64_t)freqf;
            break;
        }
    }

    fclose(fd);
    return freq;
}
/*
static uint64_t
read_tsc(void)
{
    uint32_t a, d;
    __asm __volatile("rdtsc" : "=a" (a), "=d" (d));
    return ((uint64_t) a) | (((uint64_t) d) << 32);
}
*/

typedef uint64_t atomic_t;

#define LOCK_PREFIX \
		".section .smp_locks,\"a\"\n"	\
		"  .align 4\n"			\
		"  .long 661f\n" /* address */	\
		".previous\n"			\
	       	"661:\n\tlock; "

/*
static __inline__ void
atomic_add(int i, uint64_t *v)
{
    __asm__ __volatile__(
    LOCK_PREFIX "addl %1,%0"
    :"+m" (v)
    :"ir" (i));
}
*/








#endif //NVSTREAM_UTIL_H

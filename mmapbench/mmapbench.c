/**
 * @file   mmap.c
 * @modified by Sudarsun Kannan <sudarsun.kannan@gmail.com>
 * @author Wang Yuanxuan <zellux@gmail.com>
 * @date   Fri Jan  8 21:23:31 2010
 * 
 * @brief  An implementation of mmap bench mentioned in OSMark paper
 * 
 * 
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#include "utils.h"
#include "config.h"
#include "bench.h"

/*USE posix-based file*/
#define USEFILE

#define MAP_FLAGS MAP_ANONYMOUS | MAP_PRIVATE

/*Mnemosyne flags */
//#define MAP_SCM 0x80000
//#define MAP_PERSISTENT MAP_SCM
//#define MAP_FLAGS MAP_SHARED |MAP_PERSISTENT


int nbufs = 128000;
char *shared_area = NULL;
int flag[32];
int ncores = 1;
char *filename = "/mnt/pmfs/yumamapbench";



#if defined(__YUMA)

char *data_chunk = NULL; // data chunk to be copied
unsigned long stepsize, chunksize, filesize;

void *
memcpy_worker(void *args)
{
    int id = (long) args;
    uint64_t i;

    affinity_set(id);

	uint64_t nsteps = stepsize/chunksize;
	uint64_t nchunks = filesize/chunksize;

    for (i = 0; i < nchunks; i++){
	    memcpy(&shared_area[i*chunksize],data_chunk,chunksize);
            //TODO: experiment point
            flush_clflush(&shared_area[i*chunksize],chunksize);
	}
    return (void *) 0;
}

#elif defined(__YUMA_STREAM)

char *data_chunk = NULL; // data chunk to be copied
unsigned long stepsize, chunksize, filesize;


void *
streamcpy_worker(void *args)
{
    int id = (long) args;
    uint64_t i;

    affinity_set(id);

    uint64_t nsteps = stepsize/chunksize;
    uint64_t nchunks = filesize/chunksize;

    //char *aligned_shared_area = (char *)(((uintptr_t)shared_area + 15) & 15); // aligning the mapped memory to next 16 bytes
    char *aligned_shared_area =  (((16-1) & (uintptr_t)shared_area) ? (((uintptr_t)shared_area + 16) & ~(16-1)):shared_area);

    for (i = 0; i < nchunks; i++){
        streaming_memcpy(&aligned_shared_area[i*chunksize],data_chunk,chunksize);
        //TODO: experiment point
        _mm_sfence();

    }
    return (void *) 0;

}
#endif

void *
worker(void *args){
    int id = (long) args;
    int ret = 0;
    int i;

    affinity_set(id);

    for (i = 0; i < nbufs; i++)
        ret += shared_area[i *4096];
    //printf("potato_test: thread#%d done.\n", core);
    return (void *) (long) ret;
}

struct nvmap_arg_struct{

        unsigned long fd;
        unsigned long offset;
        int chunk_id;
        int proc_id;
        int pflags;
        int noPersist;
        int refcount;
};



int create_file(char* filename, size_t bytes) {

    int fd = -1;
    int result =0;	
    
    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
                perror("Error opening file for writing");
                exit(EXIT_FAILURE);
    }
    result = lseek(fd, bytes, SEEK_SET);
    if (result == -1) {
	    close(fd);
    	perror("Error calling lseek() to 'stretch' the file");
	    exit(EXIT_FAILURE);
    }
    result = write(fd, "", 1);
    if (result != 1) {
        close(fd);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }
    printf("going to mmap \n");
	close(fd);

	return 0;
}

unsigned long tonum(char *ptr){

    char str[100];
    snprintf(str,100,"%s", ptr);
    char c = str[strlen(str) - 1];
    int temp = strlen(str);
    switch (c) {
        case 'k':
            str[temp - 1] = 0;
            return atol(str) * 1024;
        case 'm':
            str[temp - 1] = 0;
            return atol(str) * 1024 * 1024;
        case 'g':
            str[temp - 1] = 0;
            return atol(str) * 1024 * 1024 * 1024;
        default:
            return atol(str);
    }
}

int
main(int argc, char **argv)
{
    int i, fd;
    pthread_t tid[32];
    uint64_t start, end, usec , sec;
    struct nvmap_arg_struct a;
    unsigned long offset = 0;
    int chunk_id = 120;
    int proc_id = 20;

    for (i = 0; i < ncores; i++) {
        flag[i] = 0;
    }
#if defined(__YUMA) || defined(__YUMA_STREAM)
    int c, err=0;
    extern char *optarg;
    extern int optind;
    char *stepsizestr, *chunksizestr,*filesizestr;



    while((c = getopt(argc,argv,"t:s:c:")) != -1){
        switch (c){
            case 't':
                filesizestr = optarg;
                break;
            case 's':
                stepsizestr = optarg;
                break;
            case 'c':
                chunksizestr = optarg;
                break;
            case '?':
                err = 1;
                break;
        }
    }

    filesize = tonum(filesizestr);
    chunksize = tonum(chunksizestr);
    stepsize = tonum(stepsizestr);

    //debug
    printf("file size  :  %s  ,  %ld \n", filesizestr,filesize);
    printf("step size  :  %s  ,  %ld \n", stepsizestr,stepsize);
    printf("chunk size :  %s  ,  %ld \n", chunksizestr,chunksize);
    fflush(stdout);

    if(chunksize % 16){
        log_err("chunks size should be multiples of 16");
        exit(-1);
    }
#else
    if (argc > 1) {
        ncores = atoi(argv[1]);
    }
#endif
#ifdef USEFILE
#if  defined(__YUMA)
    create_file(filename, filesize);
    fd = open(filename, O_RDWR);
#elif defined(__YUMA_STREAM)
    create_file(filename, filesize+4096);
    fd = open(filename, O_RDWR);
#else
    create_file(filename, (1 + nbufs) * 4096);
    fd = open(filename, O_RDONLY);
    a.fd = fd;
#endif
#else
    a.fd = -1;	 
#endif

    a.offset = offset;
    a.chunk_id =chunk_id;
    a.proc_id = proc_id;
    a.pflags = 1;
    a.noPersist = 0;

#if defined(__YUMA)
    shared_area = mmap(NULL,filesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
#elif  defined(__YUMA_STREAM)
    shared_area = mmap(NULL,filesize+4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
#else
    shared_area = mmap(0, (1 + nbufs) * 4096, PROT_READ, MAP_FLAGS, fd, 0);
#endif

    if (shared_area == MAP_FAILED) {
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }
    printf("map %lu\n", (unsigned long)shared_area);

#if defined(__YUMA) || defined(__YUMA_STREAM)
	//data_chunk = malloc(chunksize);
    int ret = posix_memalign((void **)&data_chunk, 16 , chunksize);
    if (ret) {
        perror("Error allocating aligned memory");
        exit(EXIT_FAILURE);
    }
	memset(data_chunk,1, chunksize);
#endif

    uint64_t k;
    uint64_t dummy = 0;
    for(k=0; k< filesize; k=k+2048){
        dummy += shared_area[k];
    }
    printf("dummy value access to preload pages : %ld\n", dummy );
    start = read_tsc();
    for (i = 0; i < ncores; i++) {
#if defined(__YUMA)
        pthread_create(&tid[i], NULL, memcpy_worker, (void *) (long) i);
        log_info("creating worker for YUMA");
#elif defined(__YUMA_STREAM)
        pthread_create(&tid[i], NULL, streamcpy_worker, (void *) (long) i);
        log_info("creating worker for YUMA stream");
#else
        pthread_create(&tid[i], NULL, worker, (void *) (long) i);
        log_info("creating worker for regular");
#endif

    }

    for (i = 0; i < ncores; i++) {
        pthread_join(tid[i], NULL);
    }
    
    end = read_tsc();
    usec = (end - start) * 1000000 / get_cpu_freq();

    uint64_t mdata = filesize / (1024*1024); // in Mb

    float mbs = mdata * 8 * 1000000/(float)usec;

    printf("mb/s : %f\n",mbs);
    printf("usec: %ld\t\n", usec);



    close(fd);
    return 0;
}

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

#include "config.h"
#include "bench.h"

/*USE posix-based file*/
#define USEFILE

#define MAP_FLAGS MAP_ANONYMOUS | MAP_PRIVATE

/*Mnemosyne flags */
//#define MAP_SCM 0x80000
//#define MAP_PERSISTENT MAP_SCM
//#define MAP_FLAGS MAP_SHARED |MAP_PERSISTENT

/*pVM related flags*/
//#define MAP_FLAGS MAP_PRIVATE
//#define __NR_nv_mmap_pgoff     301

 int nbufs = 128000; 
char *shared_area = NULL;
int flag[32];
int ncores = 1;
//char *filename = "/mnt/pmfs/share.dat";
char *filename = "/mnt/mem/share.dat";

#ifdef __YUMA
uint64_t filesize = 1 * 1024 * 1024;  //1MB
uint64_t mmapsize = 1 * 1024 * 1024 *1024; // 1 GB
int iosize = -1; // provided as a cmd argument
void *data_chunk = NULL; // data chunk to be copied
#endif

void *
memcpy_worker(void *args)
{
    int id = (long) args;
    int ret = 0;
    uint64_t i;
	uint64_t j;

    affinity_set(id);

	uint64_t nwrites = mmapsize/filesize;
	uint64_t nchunks = filesize/iosize;
	assert(iosize != -1);

    for (i = 0; i < nwrites; i++){
		for(j=0; j < nchunks; j++){ 
			memcpy(&shared_area[i*filesize + j*iosize],data_chunk,iosize);
		}
		//msync
	}
    //printf("potato_test: thread#%d done.\n", core);
    return (void *) 0;
}

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


int
main(int argc, char **argv)
{
    int i, fd;
    pthread_t tid[32];
    uint64_t start, end, usec;
    struct nvmap_arg_struct a;
    unsigned long offset = 0;
    int chunk_id = 120;
    int proc_id = 20;

    for (i = 0; i < ncores; i++) {
        flag[i] = 0;
    }
#ifdef __YUMA
    if (argc > 1) {
        ncores = atoi(argv[1]);
    }
#else
    if (argc > 1) {
        iosize = atoi(argv[1]);
    }
#endif
#ifdef USEFILE
#ifdef _YUMA
    create_file(filename, (1 + nbufs) * 4096);
    fd = open(filename, O_RDONLY);
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
    shared_area = mmap(0, (1 + nbufs) * 4096, PROT_READ, MAP_FLAGS, fd, 0);

#ifdef _YUMA
	data_chunk = malloc(iosize);
	memset(data_chunk,1, iosize);
#endif
    //shared_area = (char *) syscall(__NR_nv_mmap_pgoff, 0,(1 + nbufs) * 4096,  PROT_READ | PROT_WRITE, MAP_PRIVATE| MAP_ANONYMOUS, &a);
    if (shared_area == MAP_FAILED) {
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
     }   
     printf("map %lu\n", (unsigned long)shared_area);
         
    start = read_tsc();
    for (i = 0; i < ncores; i++) {
#ifdef __YUMA
        pthread_create(&tid[i], NULL, memcpy_worker, (void *) (long) i);
#else
        pthread_create(&tid[i], NULL, worker, (void *) (long) i);
#endif
    }

    for (i = 0; i < ncores; i++) {
        pthread_join(tid[i], NULL);
    }
    
    end = read_tsc();
    usec = (end - start) * 1000000 / get_cpu_freq();
    printf("usec: %ld\t\n", usec);

    close(fd);
    return 0;
}

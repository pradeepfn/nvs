#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>
#include "phoenix.h"
#include "px_remote.h"


int main(int argc, char **argv) {
    //int i, j, rank, nranks, peer, bufsize, errors;
    //int count[2], src_stride, trg_stride, stride_level;
    int members[2];
    int cnt=2;
    int errors=-1;
    size_t bytes = 0;
    void **memory_grid;
    void *ptr;
	int myrank;
	int nranks;
	


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    remote_init(myrank,nranks);
    char string[]= "Hello world";
    bytes = 20;
    ptr = (char *)alloc_remote(&memory_grid, bytes);
    if(!ptr){errors = -1;}

    //copying to my local memory
    strncpy(ptr,string,15);
    remote_write(myrank, memory_grid,bytes);
    remote_finalize();
    MPI_Finalize();
    if (errors == 0) {
        printf("%d: Success\n", myrank);
        return 0;
    } else {
        printf("%d: Fail\n", myrank);
        return 1;
    }
}

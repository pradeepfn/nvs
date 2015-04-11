#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>
#include "phoenix.h"
#include "px_remote.h"


int main(int argc, char **argv) {

    int cnt=2;
    int errors=-1;
    size_t bytes = 0;
    void **memory_grid;
    void *ptr;
	void *dptr;
	int myrank;
	int nranks;
	


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
	printf("done with MPI init...\n");
    remote_init(myrank,nranks);

    char string[]= "Hello world";
	char secondstr[] = "Bello world";
	char thirdstr[] = "Mello world";

    bytes = 20;
    ptr = (void *)remote_alloc(&memory_grid, bytes);
	dptr = (void *) malloc(bytes);

	//printf("memory_grid[0] = %p \n", memory_grid[0]);
	//printf("memory_grid[1] = %p \n", memory_grid[1]);


    //copying to my local memory
	
    if(!myrank){
		strncpy(ptr,string,15);
	}else{
		strncpy(dptr,thirdstr,15);
		strncpy(ptr,secondstr,15);
	}

	if(!myrank){
		printf("rank : %d memory_grid[0] = %s \n", myrank, (char *)memory_grid[0]);
	}else{
		printf("rank : %d memory_grid[1] = %s \n", myrank, (char *)memory_grid[1]);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if(!myrank){
		printf("copying to remote memory from rank %d\n", myrank);
		remote_write(ptr, memory_grid,bytes);
	}else{

		remote_write(dptr, memory_grid,bytes);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	if(!myrank){
		printf("rank : %d memory_grid[0] = %s \n", myrank, (char *)memory_grid[0]);
	}else{
		printf("rank : %d memory_grid[1] = %s \n", myrank, (char *)memory_grid[1]);
	}
	remote_free(ptr);
    remote_finalize();
    MPI_Finalize();
    printf("%d: Success\n", myrank);
    return 0;
}

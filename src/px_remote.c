#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <armci.h>
#include <sys/time.h>
#include "px_remote.h"
#include "px_checkpoint.h"
#include "px_debug.h"


ARMCI_Group  g_world, my_grp;
/*Group and mpi ranks need to be same*/
int grp_my_rank, myrank, mypeer;
int nranks;
int grp_nproc;

int create_group ( int *members, int nmembers, int myrank,  int numrank);
void** group_create_memory(int nranks, size_t size);
int armci_remote_memcpy(void *src, int mypeer_rank,
				void **rmt_armci_ptr, size_t size);
int get_mypeer_group(int my_grp_rank);

int remote_barrier() {
	ARMCI_Barrier();
	return 0;
}

/*We assume MPI_Init has already been invoked
 * If not, just add this line MPI_Init*/
int remote_init(int my_rank, int n_rank) {

	/*For now lets assume 1 buddy for each node*/
	int no_members=2;
	int members[2];
	int errors=0;
	
	myrank = my_rank;
	nranks = n_rank;
	errors = ARMCI_Init();

	if(myrank % 2 == 0) {
		mypeer = myrank +1;
	}else {
		mypeer = myrank -1;
	}
	members[myrank % no_members] = myrank;
	members[mypeer % no_members] = mypeer;
//	if(isDebugEnabled()){
//		printf("Rank %d , members[%d] = %d \n",myrank,myrank%no_members,myrank);
//		printf("Rank %d , members[%d] = %d \n",myrank,mypeer%no_members,mypeer);
//	}
	/*registering with armci-lib about our grouping*/
	create_group(members, no_members, myrank, nranks);
	return errors;
}


/*
	allocate remote memory.
	returns rmtptrs
		rmtprtrs[myrank] - 
*/
void* remote_alloc(void ***memory_grid, size_t size){

	void **rmtptrs;
	rmtptrs = group_create_memory(nranks, size);
	*memory_grid = rmtptrs;
//	if(isDebugEnabled()){
//		printf("rmtptrs[0] = %p\n", rmtptrs[0]);
//		printf("rmtptrs[1] = %p\n", rmtptrs[1]);
//	}
	return rmtptrs[grp_my_rank];
}

int remote_free(void *ptr){
	int status = ARMCI_Free_group(ptr,&my_grp); 
	if(status){
		printf("Error : freeing memory");
		assert(0);
	}
	return 0;
}


int remote_write(void *src,void **memory_grid, size_t size){
	int peer = get_mypeer_group(grp_my_rank);
	//if(isDebugEnabled()){
	//	printf("Writing data local_rank: %d  remote_rank : %d remote addr : %p "  
	//				"local src addr : %p \n",myrank, mypeer, memory_grid[peer],src);
	//}
	int status = ARMCI_Put(src,memory_grid[peer],size,mypeer);
	if(status){
		printf("Error: copying data to remote node.\n");
		assert(0);	
	}
	return 0;
}

int remote_read(void *dest, void **memory_grid, size_t size){
	int peer = get_mypeer_group(grp_my_rank);
	//if(isDebugEnabled()){
	//	printf("Reading data local_rank:%d remote_rank : %d remote addr : %p " 
	//				 "local dest addr : %p \n", myrank, mypeer, memory_grid[peer],dest);
	//}
	int status = ARMCI_Get(memory_grid[peer],dest,size,mypeer);
	if(status){
		printf("Error: copying data from remote node.\n");
		assert(0);	
	}
	return 0;
}


/* free up resources */
int remote_finalize(void){
	ARMCI_Finalize();
	return 0;
}




int create_group ( int *members, int nmembers, int myrank,  int numrank) {
	if(isDebugEnabled()){
		printf("Creating a member group..\n");
	}
	ARMCI_Group_get_world(&g_world);
	ARMCI_Group_create_child(nmembers, members, &my_grp, &g_world);
	ARMCI_Group_rank(&my_grp, &grp_my_rank);
	ARMCI_Group_size(&my_grp, &grp_nproc);
	if(isDebugEnabled()){
		printf("Done creating the group..\n");
	}
	return 0;
}

/* Function to group allocate a chunk.
 */
void** group_create_memory(int nranks, size_t size) {
	armci_size_t u_bytes = size;	
	void **rmt_armci_ptr;

	rmt_armci_ptr = (void **) calloc(nranks,sizeof(void *));
	int status = ARMCI_Malloc_group(rmt_armci_ptr, u_bytes, &my_grp);
	if(status){
		printf("Error: creating group memory\n");
		assert(0);
	}
	ARMCI_Barrier();
	assert(rmt_armci_ptr);
	return rmt_armci_ptr;
}

int get_mypeer_group(int my_grp_rank){
	int gpeer_rank = 0;
	if(my_grp_rank == 0){
		gpeer_rank = my_grp_rank + 1;
		return gpeer_rank;
	}
	gpeer_rank = my_grp_rank - 1;
	return gpeer_rank;	
}


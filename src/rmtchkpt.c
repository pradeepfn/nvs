/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

//#ifdef _ARMCI_CHECKPOINT

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <armci.h>
#include <sys/time.h>
#include "rmtchkpt.h"
#include "px_checkpoint.h"

//#define _BW_PROFILING
#ifdef _BW_PROFILING
int send_cnt = 0;
#endif

/**************GLOBAL Variables**********/
int grp_nproc;
ARMCI_Group  g_world, my_grp;
/*Group and mpi ranks need to be same*/
int grp_my_rank, myrank, mypeer;
int nranks;
/**************GLOBAL Variables**********/


int invoke_barrier() {
	ARMCI_Barrier();
	//ARMCI_Access_begin(buffer[rank]);
	return 0;
}

void** create_memory(int numranks, int myrank, size_t bytes) {

	armci_size_t u_bytes;	
	void **rmt_armci_ptr;

#ifdef _BW_PROFILING
	struct timeval start,end;
	long armci_malloc_time;

	if(myrank == 0)
		gettimeofday(&start,NULL);
#endif

	rmt_armci_ptr = (void **) calloc(sizeof(void *), numranks);
	assert(rmt_armci_ptr);
	u_bytes = bytes;
	ARMCI_Malloc(rmt_armci_ptr, u_bytes);
	ARMCI_Barrier();
	//ARMCI_Access_begin(rmt_armci_ptr[myrank]);

#ifdef _BW_PROFILING
	/*Timing code*/
	if(myrank == 0){
		gettimeofday(&end,NULL);
		armci_malloc_time =simulation_time(start, end );
		fprintf(stdout,"armci_malloc_time %ld \n",armci_malloc_time);
	}
#endif
	return rmt_armci_ptr;
}

/* Function to group allocate a chunk.
 * TODO: Use ARMCI_Barriers after allocation
 * to ensure every one in the group has allocated memory.
 */
void** group_create_memory(int numranks, int myrank, size_t bytes) {

	armci_size_t u_bytes;	
	void **rmt_armci_ptr;

#if _BW_PROFILING
	struct timeval start,end;
	long armci_malloc_time;

	if(myrank == 0)
		gettimeofday(&start,NULL);
#endif

	rmt_armci_ptr = (void **) malloc(sizeof(void *) *numranks);
	assert(rmt_armci_ptr);
	u_bytes = bytes;

	ARMCI_Malloc_group(rmt_armci_ptr, u_bytes, &my_grp);
	ARMCI_Barrier();

#if _BW_PROFILING
	if(myrank == 0){
		gettimeofday(&end,NULL);
		armci_malloc_time =simulation_time(start, end );
		fprintf(stdout,"armci_malloc_time %ld \n",armci_malloc_time);
	}
#endif

	return rmt_armci_ptr;
}


int coordinate_chunk(int chunk, int mypeer, int myrank) {

	MPI_Status stat;
	int recv_chunk;

	if(myrank == 0){
		MPI_Send(&chunk, 1, MPI_INT, mypeer, 1, MPI_COMM_WORLD);
	}else {
		MPI_Recv(&recv_chunk, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &stat);
	}

	return recv_chunk;
}


int create_group ( int *members, int cnt, int myrank,  int numrank) {

	ARMCI_Group_get_world(&g_world);
	ARMCI_Group_create_child(cnt, members, &my_grp, &g_world);
	ARMCI_Group_rank(&my_grp, &grp_my_rank);
	ARMCI_Group_size(&my_grp, &grp_nproc);

#ifdef ARMCI_DEBUG
	fprintf(stdout,"myrank %d, mygroup rank %d "
			"num group proc %d\n",
			myrank, grp_my_rank,grp_nproc);
	for(int i=0; i < cnt; i++)
		fprintf(stdout,"member[%d]:%d\n",i,members[i]);

	fprintf(stdout,"====================\n");
#endif
	return 0;
}

int armci_remote_memcpy(int myrank, int my_peer,
		void **rmt_armci_ptr, size_t bytes){

	int group_peer = 0;

#ifdef _BW_PROFILING
	struct timeval start, end;
#endif


	/*grp_my_rank, group_peer set when creating
	 * a group. These are not same as global
	 * rank and peer
	 */
	if(grp_my_rank == 0)
		group_peer = grp_my_rank + 1;
	else
		group_peer = grp_my_rank - 1;


#ifdef _BW_PROFILING
		gettimeofday(&start,NULL);
#endif
		ARMCI_Put(rmt_armci_ptr[grp_my_rank],
				rmt_armci_ptr[group_peer],
				bytes,my_peer);

#ifdef _BW_PROFILING
		send_cnt++;
		gettimeofday(&end,NULL);
		add_bw_timestamp(send_cnt,start, end, bytes);
#endif
		ARMCI_Barrier();

return 0;
}



#ifdef _ARMCI_CHECKPOINT
void* alloc_remote(entry_t *chunk){

	void **rmtptrs;

	rmtptrs = group_create_memory(nranks, myrank,  chunk->size);
	assert(rmtptrs);

	/*ptr list in all members for this chunk*/
	chunk->rmt_ptr = rmtptrs;

	/*We return only 1 buddy location now*/
	return chunk->rmt_ptr[myrank];
}


int copy_to_remote(entry_t *chunk){

	if(chunk && chunk->rmt_ptr) {
		armci_remote_memcpy(myrank, mypeer,
				chunk->rmt_ptr, chunk->size);
		invoke_barrier();
	}else {
		return -1;
	}
	return 0;
}


/*We assume MPI_Init has already been invoked
 * If not, just add this line MPI_Init*/
int remote_init(void) {

	/*For now lets assume 1 buddy for each node*/
	int members[2];
	int cnt=2;
	int errors=-1;

	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &nranks);
	errors = ARMCI_Init();

	/*We assume the even and odd numbered ranks
	 * are peers
	 */
	if(myrank % 2 == 0) {
		mypeer = myrank +1;
		members[0] = myrank;
		members[1] = mypeer;

	}else {
		mypeer = myrank -1;
		members[0] = mypeer;
		members[1] = myrank;
	}

	/*registering with armci-lib about our grouping*/
	create_group(members, cnt, myrank, nranks);

	return errors;
}

int remote_finalize(void){
	ARMCI_Finalize();
	return 0;
}
#endif




#if 0
/*Test standalone code for testing
 *
 */
int main(int argc, char **argv) {

	//int i, j, rank, nranks, peer, bufsize, errors;
	//int count[2], src_stride, trg_stride, stride_level;
	int members[2];
	int cnt=2;
	int errors=-1;
	size_t bytes = 0;
	void **mybuffer;

	/*if mpi ranks are not initialized already*/
#ifdef MPIRANKS_INIT
	MPI_Init(&argc, &argv);
#endif

	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &nranks);
	ARMCI_Init();

	/*We assume the even and odd numbered ranks
	 * are peers
	 */
	if(myrank % 2 == 0) {
		mypeer = myrank +1;
		members[0] = myrank;
		members[1] = mypeer;

	}else {
		mypeer = myrank -1;
		members[0] = mypeer;
		members[1] = myrank;
	}

	/*registering with armci-lib about our grouping*/
	create_group (members, cnt, myrank, nranks);

	bytes = 1024*1024*10;
	mybuffer = group_create_memory(nranks, myrank,  bytes);
	if(!mybuffer){
		errors = -1;
	}

	armci_remote_memcpy(myrank, mypeer,mybuffer, mybuffer[myrank],bytes);

	ARMCI_Finalize();
	MPI_Finalize();

	if (errors == 0) {
		printf("%d: Success\n", myrank);
		return 0;
	} else {
		printf("%d: Fail\n", myrank);
		return 1;
	}
}
#endif



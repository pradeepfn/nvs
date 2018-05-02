// ************************************************************************
//
// miniAMR: stencil computations with boundary exchange and AMR.
//
// Copyright (2014) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
// retains certain rights in this software.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
// Questions? Contact Courtenay T. Vaughan (ctvaugh@sandia.gov)
//                    Richard F. Barrett (rfbarre@sandia.gov)
//
// ************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <assert.h>

#include "block.h"
#include "comm.h"
#include "proto.h"
#include "timer.h"
#include "wrapper.h"


int get_nblocks()
{
	int tot = 0, m;
	for (m = 0 ; m < max_num_blocks; m++)
	{
		if (blocks[m].number >= 0) // valid block
		{
			tot++;
		}
	}
	return tot;
}


double timer(void)
{
	return (MPI_Wtime());
}

int var_counter = 0;
void *ma_malloc(size_t size, char *file, int line)
{
	void *ptr;
	ptr = (void *) malloc(size);
	malloc_counter++;
	//if(size > 4096){
		//printf("object of size : %ld , from in %s at %d\n",size, file, line);
	//}
	if (ptr == NULL)
	{
		printf("NULL pointer from malloc call in %s at %d\n", file, line);
		exit(-1);
	}

	counter_malloc++;
	size_malloc += (double) size;

	return (ptr);
}

void *nvsma_malloc(size_t size, char *file, int line)
{
	void *ptr;
	char varname[20];
	snprintf(varname, sizeof(varname), "var%d", var_counter++);

	ptr = nvs_alloc(size, varname);
	malloc_counter++;
	if (ptr == NULL)
	{
		printf("NULL pointer from malloc call in %s at %d\n", file, line);
		exit(-1);
	}

	counter_malloc++;
	size_malloc += (double) size;

	return (ptr);
}

/* allocates and initialize 3d matrix in a continuous memory block */
double ***bulk_malloc(int x, int y, int z, char *file, int line){
	double ***var;
	unsigned long size =  z * sizeof(*var) + z*(y*sizeof(**var)) + z*y*(x*sizeof(***var));
	//printf("size : %ld \n", size);	
#ifdef _YUMA
	double ***ptr = (double ***)nvsma_malloc(size,file,line);
#else
	double ***ptr = (double ***)ma_malloc(size,file,line);
#endif

	double ***x_start =  ptr;
	double **y_start = (double **)(x_start + x);

	int i;
	for(i=0; i < x; i++){
		ptr[i] = y_start + i*y;
	}
	
	double *z_start =(double *)(y_start + x*y);
	
	int j,k;
	for(j=0;j<x;j++){
		for(k=0;k<y;k++){
			ptr[j][k] = z_start + (j*y*z) + (k*z);
		}
	}	
	return ptr;
}





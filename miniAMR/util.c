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
#include "phoenix.h"


int get_nblocks(){
	int tot=0, m;
	for (m=0 ; m < max_num_blocks; m++){
		if(blocks[m].number >= 0){ // valid block
			tot++;
		}
	}
	return tot;
}


double timer(void)
{
	return(MPI_Wtime());
}

int var_counter=0;
void *ma_malloc(size_t size, char *file, int line)
{
	void *ptr;
	char varname[20];
	snprintf(varname,sizeof(varname),"var%d",var_counter);
	//if(size < 27648000 || size > 27648004){
	if(size < 4096){
		ptr = (void *) malloc(size);
		malloc_counter++;
	}else{
#ifdef _YUMA
		px_obj temp;
		px_create(varname,size,&temp);
		assert(size == temp.size);
		ptr = temp.data;
		var_counter++;
	//	printf("allocating memory of size %ld\n",size);
#else
		ptr = (void *) malloc(size);
		malloc_counter++;
#endif
	}

	if (ptr == NULL) {
		printf("NULL pointer from malloc call in %s at %d\n", file, line);
		exit(-1);
	}

	counter_malloc++;
	size_malloc += (double) size;

	return(ptr);
}

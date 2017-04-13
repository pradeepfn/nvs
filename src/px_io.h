#ifndef PX_IO_H_
#define PX_IO_H_

#include <stdio.h>



// file path and the mpi rank
int io_init(stat_t *stobj, char *filepath, int rank_id){

	char file_name[50];
	DIR* dir = opendir(filepath);
	if(dir){
		snprintf(file_name,sizeof(file_name),"%s/slog_p%d.log",filepath,rank_id);
		stobj->fptr=fopen(file_name,"w");
		//write the column names
		fprintf(stobj->fptr,"t_total, t_iter, t_write, t_read, w_size, wd_size, r_size, rd_size\n");
		fflush(stobj->fptr);
	}else{
		printf("Error: no output directory found.\n\n");
		assert(0);
	}


	return 0;
}

// the stat object is defined by px itself
int io_write(stat_t *stobj){
	//write out the stobj content
	fprintf(stobj->fptr, "%lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",stobj->t_total,
			stobj->t_iter,stobj->t_write, stobj->t_read, stobj->w_size,
			stobj->wd_size, stobj->r_size, stobj->rd_size );
	fflush(stobj->fptr);
}

//shutdown io system
int io_finalize(stat_t *stobj){
	fclose(stobj->fptr);

}

#endif

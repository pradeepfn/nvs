#include <stdio.h>
#include <stdlib.h>

#include "px_log.h"
#include "px_debug.h"


int main(int argc, char **argv){
	char index_file[100],data_file[100];
	char *ret_ptr;
	//read the command line arguments
	if(argc < 3){
		printf("usage format <path> <node_id> <size-in_MB>\n");
		exit(1);
	}
	char *dir_loc = argv[1];
	ulong node_id = strtol(argv[2],&ret_ptr,10);
	ulong size = strtol(argv[3],&ret_ptr,10);

	sprintf(index_file,"%s/%s%lu",dir_loc,"mmap.file.meta",node_id);
	sprintf(data_file,"%s/%s%lu",dir_loc,"mmap.file.data",node_id);	
	log_info("shm file-names : %s , %s\n",index_file, data_file);
	log_info("shm format size : %lu MB\n", size);

	int ret = create_shm(index_file, data_file, size);	
	check(ret == 0,"error creating shm region");
	log_info("successfully created and formatted shm region");
	return 0;

}

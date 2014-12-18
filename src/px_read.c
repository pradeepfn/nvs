#include <stdio.h>
#include <stdlib.h>


void *copy_read(char *var_name,int process_id){
	void *buffer=NULL;
    void *data_addr = NULL;
    checkpoint_t *checkpoint = get_latest_version(var,id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	data_addr = get_data_addr(current->meta,checkpoint);
    buffer = malloc(checkpoint->data_size);
	memcpy(buffer,data_addr,checkpoint->data_size);
	return buffer;
}

void *precopy_read(char *var_name, int process_id){

	return NULL;
}


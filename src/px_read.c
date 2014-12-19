#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "px_log.h"
#include "px_checkpoint.h"
#include "px_debug.h"
#include "px_util.h"

void *copy_read(log_t *log, char *var_name,int process_id){
	void *buffer=NULL;
    void *data_addr = NULL;
	checkpoint_t *cbptr = log->current->meta;
    checkpoint_t *checkpoint = log_read(log,var_name,process_id);
    if(checkpoint == NULL){ // data not found
        printf("Error data not found");
        assert(0);
        return NULL;
    }
	data_addr = get_data_addr(cbptr,checkpoint);
    buffer = malloc(checkpoint->data_size);
	nvmmemcpy_read(buffer,data_addr,checkpoint->data_size);
	return buffer;
}

void *precopy_read(char *var_name, int process_id){

	return NULL;
}


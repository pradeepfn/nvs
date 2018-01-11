#include <assert.h>
#include <stdio.h>
#include "phoenix.h"


int px_init_(int *proc_id){
    return px_init(*proc_id);
}

void *alloc(unsigned long *n, char *s) {
//	printf("variable size- %lu , name- %s\n",*n,s);
    px_obj temp;
    px_create(s,(int)*n,&temp);
	assert(*n == temp.size);
	return temp.data;
}

void px_free_(void* ptr) {
    px_delete(ptr);
}

void px_snapshot_(){
    px_snapshot();
}

int px_finalize_(){
    return px_finalize();
}


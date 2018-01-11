#include <stdlib.h>

int px_init_(int *proc_id){}


void *alloc(unsigned long *n, char *s) {
    return malloc(n);
}

void px_free_(void* ptr) {
}

void px_snapshot_(){
}

int px_finalize_(){
}
//
// Created by pradeep on 1/11/18.
//

#ifndef MICROBENCH_WRAPPER_H
#define MICROBENCH_WRAPPER_H

/* C interface for fortran apps */
#ifdef __cplusplus
extern "C" {
#endif

int nvs_init_(int *proc_id);

void *alloc(unsigned long *n, char *s);

void nvs_free_(void* ptr);

void nvs_snapshot_(int *proc_id);


int nvs_finalize_();

#ifdef __cplusplus
}
#endif



#endif //MICROBENCH_WRAPPER_H

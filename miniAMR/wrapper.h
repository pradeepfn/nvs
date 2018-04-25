//
// Created by pradeep on 1/11/18.
//

#ifndef MICROBENCH_WRAPPER_H
#define MICROBENCH_WRAPPER_H

/* C interface for our C++ library */
#ifdef __cplusplus
extern "C" {
#endif

int nvs_init(int proc_id);

void *nvs_alloc(unsigned long n, char *s);

void nvs_free(void* ptr);

void nvs_snapshot(int proc_id);


int nvs_finalize();

#ifdef __cplusplus
}
#endif



#endif //MICROBENCH_WRAPPER_H

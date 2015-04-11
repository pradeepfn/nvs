
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*call this during MPI_Initialize*/
int remote_init(int my_rank, int n_rank);

/*Remote memory allocation*/
void* remote_alloc(void ***memory_grid, size_t size);

int remote_free(void *mem_ptr);

int remote_barrier();

/*Remote memory copy*/
int remote_write(void *src, void** memory_grid,size_t size);

/*call this during MPI_Finalize*/
int remote_finalize(void);
#ifdef __cplusplus
}
#endif

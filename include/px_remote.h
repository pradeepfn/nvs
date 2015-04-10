
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*call this during MPI_Initialize*/
int remote_init(int my_rank, int n_rank);

/*Remote memory allocation*/
void* alloc_remote(void ***memory_grid, size_t size);

/*Remote memory copy*/
int remote_write(int myrank, void** memory_grid,size_t size);

/*call this during MPI_Finalize*/
int remote_finalize(void);
#ifdef __cplusplus
}
#endif

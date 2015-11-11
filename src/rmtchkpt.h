#ifndef _RMTCHKPT_H
#define _RMTCHKPT_H

#include <stdio.h>
#include <stdlib.h>
#include "px_checkpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

/*call this during MPI_Initialize*/
int remote_init(void);

/*call this during MPI_Finalize*/
int remote_finalize(void);

/*Remote memory allocation*/
void* alloc_remote(var_t *chunk);

/*Remote memory copy*/
int copy_to_remote(var_t *chunk);


#ifdef __cplusplus
}
#endif

#endif // _RMTCHKPT_H
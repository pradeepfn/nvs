#ifndef __PX_UTIL_H
#define __PX_UITL_H

#include <stdlib.h>

int memcpy_read(void *, void *, size_t);
int memcpy_write(void *, void *, size_t);
void *get_data_addr(void *, checkpoint_t *);
int timeval_subtract (struct timeval *, struct timeval *, struct timeval *);


#endif

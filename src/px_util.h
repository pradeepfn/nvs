#ifndef __PX_UTIL_H
#define __PX_UITL_H

#include <stdlib.h>
#include <assert.h>

#include "px_read.h"
#include "px_checkpoint.h"
#include "px_log.h"

int nvmmemcpy_read(void *, void *, size_t);
int nvmmemcpy_write(void *, void *, size_t);
void *get_data_addr(void *, checkpoint_t *);
int timeval_subtract (struct timeval *, struct timeval *, struct timeval *);
void put_pagemap(pagemap_t ** ,void *, void *, offset_t, offset_t);
pagemap_t *get_pagemap(pagemap_t **, void *);
int disable_protection(pagemap_t *, void *);
void install_sighandler(void (*sighandler)(int,siginfo_t *,void *));
int enable_protection(void *ptr, size_t size);



#endif

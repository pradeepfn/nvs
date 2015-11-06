#ifndef __PX_UTIL_H
#define __PX_UITL_H

#include <stdlib.h>
#include <assert.h>

#include "px_read.h"
#include "px_checkpoint.h"
#include "px_log.h"

#define MICROSEC 1000000

int nvmmemcpy_read(void *, void *, size_t);
int nvmmemcpy_write(void *, void *, size_t);
void *get_data_addr(void *, checkpoint_t *);
int timeval_subtract (struct timeval *, struct timeval *, struct timeval *);
void pagemap_put(pagemap_t **, char *, void *, void *, offset_t, offset_t, void **);
pagemap_t *pagemap_get(pagemap_t **, void *);
long disable_protection(void *addr, size_t size);
void install_sighandler(void (*sighandler)(int,siginfo_t *,void *));
void enable_protection(void *ptr, size_t size);
void copy_chunks(pagemap_t **page_map_ptr);
void copy_remote_chunks(pagemap_t **page_map_ptr);
void call_oldhandler(int signo);
int get_mypeer(int myrank);
void split_checkpoint_data(listhead_t *head);
void install_old_handler();
char* null_terminate(char *c_string);
int is_dlog_checkpoing_data_present(listhead_t *head);



#endif

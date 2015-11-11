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
long disable_protection(void *addr, size_t size);
void install_sighandler(void (*sighandler)(int,siginfo_t *,void *));
void call_oldhandler(int signo);
int get_mypeer(int myrank);
void split_checkpoint_data(var_t *list);
void install_old_handler();
int is_dlog_checkpoing_data_present(var_t *list);
void enable_write_protection(void *ptr, size_t size);
char* null_terminate(char *c_string);



#endif

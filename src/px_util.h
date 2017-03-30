#ifndef __PX_UTIL_H
#define __PX_UITL_H

#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "px_log.h"
#include "px_types.h"

#define MICROSEC 1000000

int nvmmemcpy_read(void * dest, void * src, size_t size,int rbw);
int nvmmemcpy_write(void * dest, void * src, size_t size,int wbw);
var_t *px_alighned_allocate(size_t size, char *key);
void read_configs(ccontext_t *config_context,char *file_path);
void md5_digest(unsigned char *digest,void *data,ulong length);
long get_varsize(int *,long);
long nvmmemcpy_dedupv(void *dest_ptr,void *var_ptr,long var_size,int *dvector, long dv_size, int nvram_bw);
long nvmmemcpy_dedup_apply(void *ret_ptr,long size, void *var_ptr,long var_size,int *dvector,long dv_size);
void call_oldhandler(int signo);
long disable_protection(void *page_start_addr,size_t aligned_size);
void enable_write_protection(void *ptr, size_t size);


#endif

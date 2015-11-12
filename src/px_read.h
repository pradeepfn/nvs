#ifndef __PX_READ_H
#define __PX_READ_H

#include <pthread.h>

#include "px_log.h"
#include "px_checkpoint.h"

var_t *copy_read(log_t *log, char *var_name,int process_id, long version);
#endif

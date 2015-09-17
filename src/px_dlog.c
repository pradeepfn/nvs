//
// Created by pradeep on 9/17/15.
//

#include "px_dlog.h"


/*
 * This is a dram based implementation of the volatile log. Responsible for keeping
 * both local and remote checkpoint data in memory.
 */

void dlog_init(dlog_t *dlog) {

}

int dlog_write(dlog_t *log, listhead_t *lhead, checkpoint_type type) {
    return 0;
}




dcheckpoint_t *dlog_read(dlog_t *log, char *var_name, int process_id) {
    return NULL;
}
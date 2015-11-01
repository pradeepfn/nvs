//
// Created by pradeep on 10/30/15.
//

#ifndef PHOENIX_DESTAGER_H
#define PHOENIX_DESTAGER_H

#include "px_dlog.h"
#include "px_log.h"

typedef struct destage_t_{
    dlog_t *dlog;
    log_t *nvlog;
    int process_id;
}destage_t;

void destage_data(void *args);



#endif //PHOENIX_DESTAGER_H

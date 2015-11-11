#ifndef PHOENIX_EARLYCOPY_H
#define PHOENIX_EARLYCOPY_H

#include "px_log.h"
#include "px_read.h"

typedef struct earlycopy_t_{
    log_t *nvlog; //nvram
    var_t *list; // current data

}earlycopy_t;


void start_copy(void *args);


#endif //PHOENIX_EARLYCOPY_H

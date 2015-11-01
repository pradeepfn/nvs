//
// Created by pradeep on 10/30/15.
//

#ifndef PHOENIX_EARLYCOPY_H
#define PHOENIX_EARLYCOPY_H

#include "px_log.h"

typedef struct earlycopy_t_{
 log_t *nvlog;

}earlycopy_t;


void start_copy(void *args);


#endif //PHOENIX_EARLYCOPY_H

//
// Created by pradeep on 10/27/17.
//

#ifndef NVSTREAM_LAYOUT_H
#define NVSTREAM_LAYOUT_H


#define MAX_LOGS 256

#include <libpmemobj/base.h>
#include <libpmemobj.h>

POBJ_LAYOUT_BEGIN(nvstream_store);
POBJ_LAYOUT_ROOT(nvstream_store, struct nvs_root);
POBJ_LAYOUT_END(nvstream_store);



struct nvs_root{
    //shared memory mutex to protect access to root object elements
    int length; // how many logs
    int log_id[256];
};

#endif //NVSTREAM_LAYOUT_H

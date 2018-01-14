/*
 *
 * create the root heap in a node/machine. This bootstraps the shared heap
 *
 */


#include "nvs/log.h"
#include "layout.h"


int main(int argc, char *argv[]){

    pop =  pmemobj_open(root_file_path.c_str(),
                        POBJ_LAYOUT_NAME(nvstream_store));
    if(pop != NULL){
        LOG(error) << "root heap structures found!. clean them first";
        return PMEM_ERROR;
    }

    pop = pmemobj_create(root_file_path.c_str(),
                         POBJ_LAYOUT_NAME(nvstream_store),
                         PMEMOBJ_MIN_POOL, 0666);
    if (pop == NULL){
        LOG(error) << "root.cc : error while creating root heap";
        return PMEM_ERROR;
    }
    //TODO : persist
    TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);
    D_RW(root_heap)->length = 0;

    LOG(info) << "successfully created the root heap";
    return NO_ERROR;
}
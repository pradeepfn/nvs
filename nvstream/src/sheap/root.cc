//
// Created by pradeep on 10/24/17.
//

#include <libpmemobj/base.h>
#include <libpmemobj.h>
#include <nvs/log.h>

#include "root.h"
#include "layout.h"

namespace nvs{


    RootHeap::RootHeap(std::string pathname)
            :root_file_path(pathname)
    {

    }

    RootHeap::~RootHeap()
    {

    }

     ErrorCode RootHeap::Open() {
           pop =  pmemobj_open(root_file_path.c_str(),
                               POBJ_LAYOUT_NAME(nvstream_store));
           if(pop == NULL){
               LOG(error) << "root.cc : error while opening root heap";
               return PMEM_ERROR;
           }
           return NO_ERROR;
     }

    ErrorCode RootHeap::Create() {
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

        return NO_ERROR;
    }

    ErrorCode RootHeap::addLog(PoolId id) {

        TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);


        // TODO: persist
        D_RW(root_heap)->log_id[0] = id;
        D_RW(root_heap)->length = D_RO(root_heap)->length + 1;

        return NO_ERROR;

    }

    ErrorCode RootHeap::Destroy() {

    }


    ErrorCode RootHeap::Close() {
        pmemobj_close(pop);
        pop = NULL;
    }

    bool RootHeap::isLogExist(PoolId id) {
        TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);
            for(int i =0; i < D_RO(root_heap)->length;i++){
                if(D_RO(root_heap)->log_id[i] == id){return true;}
            }
        return false;
    }
 }
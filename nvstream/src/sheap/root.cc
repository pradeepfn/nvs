//
// Created by pradeep on 10/24/17.
//

#include <libpmemobj/base.h>
#include <libpmemobj.h>

#include "root.h"

namespace nvs{

     ErrorCode RootHeap::Open() {
        pop = pmemobj_open(root_file_path, );
         if(pop == NULL){
             perror("pmemobj_open");
             return 1;
         }
         pmemobj_root(pop, sizeof(struct root_heap));
         rp = pmemobj_direct()
     }

    ErrorCode RootHeap::Create() {
        pop = pmemobj_create(root_file_path,
                                          LAYOUT_NAME, PMEMOBJ_MIN_POOL, 0666);

        if (pop == NULL){
            perror("pmemobj_create");
            return 1; // TODO
        }


    }

    ErrorCode RootHeap::Destroy() {

    }


    ErrorCode RootHeap::Close() {
        pmemobj_close(pop);
        pop = NULL;
    }


 }
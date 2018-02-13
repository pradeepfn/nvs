//
// Created by pradeep on 2/11/18.
//

#ifndef NVSTREAM_DELTA_STORE_H
#define NVSTREAM_DELTA_STORE_H

#include <nvs/memory_manager.h>
#include <map>
#include <csignal>

#include "nvs/store.h"
#include "key.h"
#include "constants.h"
#include "object.h"
#include "common/util.h"


#define PAGE_SIZE 4096

namespace nvs {

    class DeltaStore : public Store {
    private:

        std::string storeId;
        ProcessId pid;
        MemoryManager *mm; // memory manager of the metadata heap
        Log *log; // process local log
        Key *findKey(std::string key);
        objkey_t *lptr(uint64_t offset);
        std::map<std::string, Object *> objectMap;

        struct sigaction old_sa; // old sigaction structure


        void delta_memcpy(char *dst, char *src,
                        struct ledelta_t *deltas, uint64_t delta_len);

    protected:
    public:


         void delta_handler(int sig, siginfo_t *si, void *unused){
            if(si != NULL && si->si_addr != NULL){
                void *pageptr;
                uint64_t offset = 0;
                pageptr =  si->si_addr;

                std::map<std::string, Object *>::iterator it;
                for(it=objectMap.begin(); it != objectMap.end(); it++){
                    offset = (uint64_t)pageptr - (uint64_t)it->second->getPtr();
                    if(offset >=0 && offset <= it->second->get_aligned_size()){ // the address belong to this object
                        /* if(isDebugEnabled()){
                             printf("[%d] starting address of the matching chunk %p\n",lib_process_id, s->ptr);
                         }*/
                        //get the page start
                        pageptr = (void *)((long)si->si_addr & ~(PAGE_SIZE-1));
                        disable_protection(pageptr, PAGE_SIZE);

                        uint64_t page_index = ((uint64_t)pageptr & ~(PAGE_SIZE-1)) - 1; // 0 based indexing
                        it->second->set_modified_bit(page_index);
                        return;
                    }
                }
                /* the raised signal not belong to our tracking addresses */
                call_oldhandler(sig, &(this->old_sa));
            }
        }




        DeltaStore(std::string storeId);

        ~DeltaStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode put(std::string key, uint64_t version);

        ErrorCode put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        void stats();

    };

}

#endif //NVSTREAM_DELTA_STORE_H

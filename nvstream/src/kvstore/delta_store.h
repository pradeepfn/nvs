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
#define LOG_SIZE 500 * 1024 * 1024LLU

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
        std::map<uint64_t,std::string> addrMap; // address to object mapping. we use this for free

        void delta_memcpy(char *dst, char *src,
                        struct ledelta_t *deltas, uint64_t delta_len);

    protected:
    public:
        void delta_handler(int sig, siginfo_t *si, void *unused);



        std::string get_store_id(){
                	return storeId;
               }


        DeltaStore(std::string storeId);

        ~DeltaStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode free_obj(void *obj_addr);

        uint64_t put(std::string key, uint64_t version);

        uint64_t put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        void stats();

    };

}

#endif //NVSTREAM_DELTA_STORE_H

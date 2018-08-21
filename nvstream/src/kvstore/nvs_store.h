
#include <nvs/memory_manager.h>
#include <map>
#include "nvs/store.h"
#include "constants.h"
#include "object.h"
#include "common/util.h"

#ifndef NVS_STORE_H
#define NVS_STORE_H


#define PLOG_SIZE 100 * 1024 * 1024LLU // 2 GB of log space per process
#define NCOMPACTOR_THREADS 2
#define IS_COMPACTOR 0

namespace nvs {

    class NVSStore : public Store {
    private:

        std::string storeId;
        ProcessId pid;
        MemoryManager *mm; // memory manager of the metadata heap
        Log *log; // process local log
        nvs_config_t *config;
        std::map<std::string, Object *> objectMap; // runtime representation of volatile object
        std::map<uint64_t,std::string> addrMap; // address to object mapping. we use this for free


    protected:
    public:
        NVSStore(std::string storeId);

        ~NVSStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode free_obj(void *obj_addr);

        uint64_t put(std::string key, uint64_t version);

        uint64_t put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        std::string get_store_id(){
        	return storeId;
        }

        void stats();

    };

    static int log_compactor(volatile char *pmem){
        printf("log compactor ran\n");
    }


}

#endif //YUMA_STORE_H

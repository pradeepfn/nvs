
#include <nvs/memory_manager.h>
#include <map>
#include "nvs/store.h"
#include "key.h"
#include "constants.h"
#include "object.h"

#ifndef NVS_STORE_H
#define NVS_STORE_H


#define LOG_SIZE  2 *1024*1024*1024LLU // 2 GB of log space per process

namespace nvs {

    class NVSStore : public Store {
    private:

        std::string storeId;
        ProcessId pid;
        MemoryManager *mm; // memory manager of the metadata heap
        Log *log; // process local log
        Key *findKey(std::string key);
        objkey_t *lptr(uint64_t offset);
        std::map<std::string, Object *> objectMap;

    protected:
    public:
        NVSStore(std::string storeId);

        ~NVSStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode put(std::string key, uint64_t version);

        ErrorCode put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        void stats();

    };

}

#endif //YUMA_STORE_H

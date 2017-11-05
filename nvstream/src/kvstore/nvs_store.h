
#include <nvs/memory_manager.h>
#include <map>
#include "nvs/store.h"
#include "key.h"
#include "constants.h"
#include "object.h"

#ifndef YUMA_IGTStore_H
#define YUMA_IGTStore_H

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

        ErrorCode get(std::string key, uint64_t version, void **obj_addr);

    };

}

#endif //YUMA_STORE_H

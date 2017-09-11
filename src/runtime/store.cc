//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <nvs/runtimeManager.h>
#include "serializationTypes.h"

namespace nvs{

    Store::Store(uint64_t *addr, std::string storeId):addr(addr),storeId(storeId)
    {
        store_t *st = (store_t *) addr;
        snprintf(st->storeId,100,storeId.c_str());
    }

    Store::~Store() {}

    void* Store::create_obj(std::string key, uint64_t version)
    {

    }

    int Store::put(std::string key, uint64_t version)
    {

    }

    int Store::get(std::string key, uint64_t version, std::string range)
    {


    }

    ErrorCode Store::close() {}
}

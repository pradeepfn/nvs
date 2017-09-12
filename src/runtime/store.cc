//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <nvs/runtimeManager.h>
#include <common/nanassert.h>
#include "serializationTypes.h"

namespace nvs{

    Store::Store(uint64_t *addr, std::string storeId):addr(addr),storeId(storeId)
    {
        srl_store = (store_t *) addr;
        I(storeId.compare(std:string(srl_store->storeId)));
        /* we create a region to store keys for this store */
        kr_addr = rt->findKeyRegion(storeId);
    }

    Store::~Store() {}

    /* traverse the list of key structs and create a new key if not found*/
    ErrorCode Store::create_obj(std::string key, uint64_t size, uint64_t **obj_addr)
    {
        /*
         * right now the key-region is formatted as an array structure.
         */
        objkey_t *prev_st;
        for(objkey_t *st = this->lptr(kr_addr->next); st != nullptr;
                prev_st = st, st = this->lptr(st->next)){
            if(key.compare(st->keyId)){
                return DUPLICATE_KEY;
            }
        }
        //TODO: assert < mem-region-size
        objkey_t *new_st = prev_st +1;
        snprintf(new_st->keyId,100,key.c_str());
        new_st->next = -1234;

        //TODO: set the previous structure next to new one

        return NO_ERROR;
    }


    /*
     * this is similar to a commit operation. we do not need the object
     * address and size for put operation as the runtime already know that
     * detail.
     */
    ErrorCode Store::put(std::string key, uint64_t version)
    {
            Key *k = findKey(key);
            return k->createVersion(version);
    }

    /*
     *  the object address is redundant here
     */
    ErrorCode Store::get(std::string key, uint64_t size, uint64_t version,
                         uint64_t **obj_addr)
    {
        Key *k = findKey(key);
        return k->getVersion(version,obj_addr);

    }

    ErrorCode Store::close() {}


    Key * Store::findKey(std::string key) {



    }
}

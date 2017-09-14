//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <nvs/runtimeManager.h>
#include <common/nanassert.h>
#include "serializationTypes.h"
#include "gtstore.h"

namespace nvs{

    GTStore::GTStore(){}

    GTStore::GTStore(uint64_t *addr, std::string storeId):key_head((objkey_t *)addr),storeId(storeId)
    {
        srl_store = (store_t *) addr;
        I(GTStoreId.compare(std:string(srl_GTStore->GTStoreId)));
        /* we create a region to GTStore keys for this GTStore */
        //kr_addr = rt->findKeyRegion(GTStoreId);
    }

    GTStore::~GTStore() {}

    /* traverse the list of key structs and create a new key if not found*/
    ErrorCode GTStore::create_obj(std::string key, uint64_t size, uint64_t **obj_addr)
    {
        /*
         * right now the key-region is formatted as an array structure.
         */
        objkey_t *prev_st;
        for(objkey_t *st = this->lptr(key_head->next); st != nullptr;
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
    ErrorCode GTStore::put(std::string key, uint64_t version)
    {
            Key *k = findKey(key);
            return k->createVersion(version);
    }

    /*
     *  the object address is redundant here
     */
    ErrorCode GTStore::get(std::string key, uint64_t version, uint64_t **obj_addr)
    {
        Key *k = findKey(key);
        return k->getVersion(version,obj_addr);

    }

    ErrorCode GTStore::close() {}


    Key * GTStore::findKey(std::string key) {



    }

    objkey_t * GTStore::lptr(uint64_t offset) {
            return (objkey_t *)(key_head + offset);
        }
}

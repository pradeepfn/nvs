//
// Created by pradeep on 9/14/17.
//
#include "nvs/store.h"
#include "key.h"

#ifndef YUMA_IGTStore_H
#define YUMA_IGTStore_H

namespace nvs {

/*
     * Store representation. This class abstraction mutate the
     * persistent state on the NVM transparent to the application
     */
    class GTStore : public Store {
    private:
        objkey_t *key_head; // starting address of key-region

        std::string storeId;
        store_t *srl_store; // serialized verison of this store object
        RuntimeManager *rt;

        Key *findKey(std::string key);

        objkey_t *lptr(uint64_t offset);

    protected:
    public:

        GTStore();

        GTStore(uint64_t *addr, std::string storeId);

        ~GTStore();

        ErrorCode create_obj(std::string key, uint64_t size, uint64_t **obj_addr);

        ErrorCode put(std::string key, uint64_t version);

        ErrorCode get(std::string key, uint64_t version, uint64_t **obj_addr);

        ErrorCode close();
    };

}

#endif //YUMA_STORE_H

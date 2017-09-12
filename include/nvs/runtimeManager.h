//
// Created by pradeep on 8/27/17.
//

#ifndef YUMA_RUNTIMEMANAGER_H
#define YUMA_RUNTIMEMANAGER_H

#include <memory>
#include <mutex>
#include <atomic>
#include "nvs/errorCode.h"

namespace nvs{







    /*
     * Store representation. This class abstraction mutate the
     * persistent state on the NVM transparent to the application
     */
    class Store{
    private:
        objkey_t *kr_addr; // starting address of key-region

        std::string storeId;
        store_t *srl_store; // shared memory representation of the store.
        RuntimeManager *rt;

        Key *findKey(std::string key);


    protected:
    public:
        Store(uint64_t *addr, std::string storeId);
        ~Store();
        void *create_obj(std::string key, uint64_t version);
        int put(std::string key, uint64_t version);
        int get(std::string key, uint64_t version, std::string range);
        ErrorCode close();
    };






    class RuntimeManager{
    private:
        static std::atomic<RuntimeManager *> instance_;
        static std::mutex mutex_;

        RuntimeManager();
        ~RuntimeManager();

        class Impl_;
        std::unique_ptr<Impl_> privateImpl;

    protected:
    public:
        static RuntimeManager *getInstance();
        /*
         * Root of meta-data structures associated with this workflow
         */
        ErrorCode createStore(std::string storeId, Store **store);

        /*
         *  get hold of root structure
         */
        ErrorCode findStore(std::string rootId, Store **store);


        ErrorCode close();
    };


}


#endif //YUMA_RUNTIMEMANAGER_H

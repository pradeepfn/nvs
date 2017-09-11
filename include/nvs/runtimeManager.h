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



    class Version{


    public:
    };

    /*
     * Key
     */
    class Key{

    private:
        //gptr *ptr;
        uint64_t size;
        /* bit field to keep track of modified pages of the buffer */
        char *barry;
        /*vector of versions */


    public:
        Version *getVersion(uint64_t version);
        uint64_t getSize();

    };



    /*
     * Store representation. This class abstraction mutate the
     * persistent state on the NVM transparent to the application
     */
    class Store{
    private:
        uint64_t *addr;
        std::string storeId;
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

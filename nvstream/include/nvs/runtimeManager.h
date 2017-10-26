//
// Created by pradeep on 8/27/17.
//

#ifndef YUMA_RUNTIMEMANAGER_H
#define YUMA_RUNTIMEMANAGER_H

#include <memory>
#include <mutex>
#include <atomic>
#include "nvs/errorCode.h"
#include "nvs/runtimeManager.h"
#include "nvs/store.h"

namespace nvs{

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

//
// Created by pradeep on 8/29/17.
//

#include <string>
#include <atomic>
#include <mutex>
#include "nvs/runtimeManager.h"
#include "nvs/errorCode.h"
#include "nvmm/memory_manager.h"

namespace nvs{
/*
 * internal implementation of RuntimeManager, this pattern was inspired by
 * hp-nvmm
 **/
 class RuntimeManager::Impl_{

     Impl_(){ }

     ErrorCode init();
     ErrorCode finalize();

     ErrorCode createStore(std::string rootId);
     ErrorCode FindStore(std::string rootId, Root **root);

 };

    ErrorCode RuntimeManager::Impl_::init() {

    }

    ErrorCode RuntimeManager::Impl_::createStore(std::string rootId) {

    }

    ErrorCode RuntimeManager::Impl_::finalize() {



    }



    RuntimeManager::RuntimeManager():privateImpl{new Impl_}{
        ErrorCode ret = privateImpl->init();
   }

    RuntimeManager::~RuntimeManager() {
        ErrorCode ret = privateImpl->finalize();
    }

    ErrorCode RuntimeManager::findStore(std::string rootId, Root **root) {
        return privateImpl->findStore(rootId,root);
    }

    ErrorCode RuntimeManager::createStore(std::string rootId) {

        return privateImpl->createStore(rootId);

    }

    std::atomic<RuntimeManager *> RuntimeManager::instance_;
    std::mutex RuntimeManager::mutex_;

    RuntimeManager* RuntimeManager::getInstance() {
       RuntimeManager *tmp = instance_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            tmp = instance_.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
                tmp = new RuntimeManager;
                std::atomic_thread_fence(std::memory_order_release);
                instance_.store(tmp, std::memory_order_relaxed);
            }
        }
        return tmp;
    }
}

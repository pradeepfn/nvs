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

    nvmm::Region *region;
     nvmm::MemoryManager *mm;
     uint64_t *mapped_addr;
     size_t size;

     Impl_(){ }

     ErrorCode init();
     ErrorCode finalize();

     ErrorCode createStore(std::string rootId,Store **store);
     ErrorCode FindStore(std::string rootId, Root **root);

 };

    ErrorCode RuntimeManager::Impl_::init() {
        this->mm = nvmm::MemoryManager::GetInstance();

        // create a new 128MB NVM region with pool id 1
        nvmm::PoolId pool_id = 1;
        this->size = 128*1024*1024; // 128MB
        nvmm::ErrorCode ret = mm->CreateRegion(pool_id, size);
        assert(ret == NO_ERROR);

        // acquire the region
        this->region = NULL;
        ret = mm->FindRegion(pool_id, &region);
        assert(ret == NO_ERROR);

        // open and map the region
        ret = region->Open(O_RDWR);
        assert(ret == NO_ERROR);
        this->mapped_addr = NULL;

        ret = region->Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&mapped_addr);
        assert(ret == NO_ERROR);

    }

    ErrorCode RuntimeManager::Impl_::createStore(std::string storeId,Store **store) {
            *store =  new Store(this->mapped_addr,storeId);
    }

    ErrorCode RuntimeManager::Impl_::finalize() {

        //TODO: delete store

        // unmap and close the region
        nvmm::ErrorCode ret = this->region->Unmap(mapped_addr, size);
        assert(ret == NO_ERROR);
        ret = this->region->Close();
        assert(ret == NO_ERROR);

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

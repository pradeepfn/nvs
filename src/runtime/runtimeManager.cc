//
// Created by pradeep on 8/29/17.
//

#include <string>
#include <atomic>
#include <mutex>
#include <map>
#include <common/nanassert.h>
#include "nvs/runtimeManager.h"
#include "nvmm/memory_manager.h"
#include "constants.h"
#include "serializationTypes.h"
#include "gtstore.h"

namespace nvs{
/*
 * internal implementation of RuntimeManager, this pattern was inspired by
 * hp-nvmm
 **/
 class RuntimeManager::Impl_{
 private:
    nvmm::Region *region;
     nvmm::MemoryManager *mm;
     store_t *st_head; // root address of shared memory stored store structures
     size_t size;
     std::map<std::string,GTStore *> storeMap; // Store objects

     store_t *lptr(uint64_t offset){
         if(offset == Constants::LIST_TERMINATOR) {return  (store_t *)offset;}
         (store_t *)(st_head + offset);
     }

 public:
     Impl_(){ }

     ErrorCode init();
     ErrorCode finalize();

     ErrorCode createStore(std::string rootId,Store **store);
     ErrorCode findStore(std::string rootId, Store **store);

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
        this->st_head = NULL;

        ret = region->Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&st_head);
        assert(ret == NO_ERROR);

    }

    /*
     * traverse through the shared memory stored store structures, if no store
     * found with given store-id then create store on shared memory, and return
     * a store object.
     */
    ErrorCode RuntimeManager::Impl_::createStore(std::string storeId,Store **store) {
        /* traverse our serialized store structure and provision new store */
        store_t *gtstore = nullptr;
        {
            store_t *st;
            int i;
            for (st = st_head, i = 0; lptr(st->next) != (store_t *) Constants::LIST_TERMINATOR;
                 st = lptr(st->next),i++);

            gtstore = st_head + i + 1;
            st->next = (i+1)*sizeof(store_t);
        }


        gtstore->next = Constants::LIST_TERMINATOR;
        gtstore->key_root = Constants::NULL_OFFSET;
        snprintf(gtstore->storeId, 100, "%s", storeId.c_str());
        GTStore *gs =  new GTStore(gtstore);
        storeMap[storeId] = gs;
        *store = gs;
        return NO_ERROR;
    }


    /*
     * traverse the local map for store, if not found then traverse the shared memory
     * segment for a store.
     */
    ErrorCode RuntimeManager::Impl_::findStore(std::string storeId, Store **store) {

        std::map<std::string, GTStore *>::const_iterator it = storeMap.find(storeId);
        if (it != storeMap.end()) {
            I(storeId.compare(it->first))
            *store = it->second;
            return NO_ERROR;
        }
        // traverse the shared memory segments to find the store
        for (store_t *st = lptr(st_head->next); st != (store_t *) Constants::LIST_TERMINATOR;
             st = lptr(st->next)) {
            if (storeId.compare(st->storeId)) {
                GTStore *tmp = new GTStore(st); // create object around shared memory structure
                storeMap[storeId] = tmp;
                *store = tmp;
                return NO_ERROR;
            }
        }
        return STORE_NOT_FOUND;
    }

    /*
     *
     */
    ErrorCode RuntimeManager::Impl_::finalize() {

        //TODO:delete store objects
        //std::map<std::string, GTStore *>::const_iterator it;

        // unmap and close the region
        nvmm::ErrorCode ret = this->region->Unmap(st_head, size);
        I(ret == NO_ERROR);
        ret = this->region->Close();
        I(ret == NO_ERROR);
        return NO_ERROR;
    }



    RuntimeManager::RuntimeManager():privateImpl{new Impl_}{
        ErrorCode ret = privateImpl->init();
   }

    RuntimeManager::~RuntimeManager() {
        ErrorCode ret = privateImpl->finalize();
    }

    ErrorCode RuntimeManager::findStore(std::string storeId, Store **store) {
        return privateImpl->findStore(storeId,store);
    }

    ErrorCode RuntimeManager::createStore(std::string storeId, Store **store) {

        return privateImpl->createStore(storeId, store);

    }

    ErrorCode RuntimeManager::close() {
        return privateImpl->finalize();
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

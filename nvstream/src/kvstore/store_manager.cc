
#include <atomic>
#include <mutex>
#include <memory>
#include "nvs/store_manager.h"
#include "nvs/store.h"

#if defined(_FILE_STORE)
#include "file_store.h"
#else
#include "nvs_store.h"
#endif

namespace nvs{


    std::atomic<Store *> StoreManager::instance_;
    std::mutex StoreManager::mutex_;

    Store * StoreManager::GetInstance(std::string storePath) {
        Store *tmp = instance_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            tmp = instance_.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
#if defined(_FILE_STORE)
                tmp = new FileStore(storePath);
#else
                tmp = new NVSStore(storePath);
#endif
                std::atomic_thread_fence(std::memory_order_release);
                instance_.store(tmp, std::memory_order_relaxed);
            }
        }
        return tmp;

    }




}

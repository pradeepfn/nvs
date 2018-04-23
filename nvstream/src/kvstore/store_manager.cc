
#include <atomic>
#include <mutex>
#include <memory>
#include "nvs/log.h"
#include "nvs/store_manager.h"

#if defined(_FILE_STORE)
#include "file_store.h"
#include "timing_store.h"
#elif defined(_DELTA_STORE)
#include "delta_store.h"
#include "timing_store.h"
#else
#include "nvs_store.h"
#include "timing_store.h"
#endif

namespace nvs{

#if defined(_DELTA_STORE)
	extern DeltaStore *ds_object;
#endif

    std::atomic<Store *> StoreManager::instance_;
    std::mutex StoreManager::mutex_;

    Store * StoreManager::GetInstance(std::string storePath) {
        Store *tmp = instance_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            tmp = instance_.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
                LOG(debug) << "testing log";
#if defined(_FILE_STORE)

#if defined (_TIMING)
                tmp = new TimingStore(new FileStore(storePath));
#else
                tmp = new FileStore(storePath);
#endif

#elif defined(_DELTA_STORE)
#if defined (_TIMING)
                DeltaStore *vtmp = new DeltaStore(storePath);
                tmp = new TimingStore(vtmp);
                ds_object = vtmp;
#else
                tmp = new DeltaStore(storePath);
                ds_object = (DeltaStore *)tmp;
#endif
#else

#if defined (_TIMING)
                tmp = new TimingStore(new NVSStore(storePath));
#else
                tmp = new NVSStore(storePath);
#endif

#endif
                std::atomic_thread_fence(std::memory_order_release);
                instance_.store(tmp, std::memory_order_relaxed);
            }
        }
        return tmp;

    }




}

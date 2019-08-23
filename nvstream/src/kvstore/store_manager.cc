
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
#elif defined(_NULL_STORE)
#include "null_store.h"
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
    std::map<std::string,Store*> m_Map;

    Store * StoreManager::GetInstance(std::string storePath) {
        Store *tmp = instance_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
	/* If there's an existing store and the names match, return it */
	if ((tmp != nullptr) && (tmp->get_store_id() == storePath)) return tmp;

	/* didn't find first store, or first store name didn't match, try again with a lock */
	std::lock_guard<std::mutex> lock(mutex_);
	tmp = instance_.load(std::memory_order_relaxed);
	if (tmp)
	    std::cout << "Found store with name " << tmp->get_store_id() << std::endl;
	if ((tmp != nullptr) && (tmp->get_store_id() == storePath)) return tmp;

	/* no first, or first name didn't match, check map */
	auto it = m_Map.find(storePath);
	if (it  != m_Map.end()) {
	    tmp = it->second;
	    return tmp;
	}

#if defined(_FILE_STORE)
                LOG(debug) << "File store enabled";
#if defined (_TIMING)
                tmp = new TimingStore(new FileStore(storePath));
#else
                tmp = new FileStore(storePath);
#endif

#elif defined(_DELTA_STORE)
                LOG(debug) << "Delta store enabled";

#if defined (_TIMING)
                DeltaStore *vtmp = new DeltaStore(storePath);
                tmp = new TimingStore(vtmp);
                ds_object = vtmp;
#else
                tmp = new DeltaStore(storePath);
                ds_object = (DeltaStore *)tmp;
#endif


#elif defined(_NULL_STORE)
                LOG(debug) << "Null store enabled";

#if defined(_TIMING)
                	tmp = new  TimingStore(new NullStore(storePath));
#else
                	tmp = new NullStore(storePath);
#endif

#else
                LOG(debug) << "NVS store enabled";

#if defined (_TIMING)
                tmp = new TimingStore(new NVSStore(storePath));
#else
                tmp = new NVSStore(storePath);
#endif

#endif
                m_Map[tmp->get_store_id()] = tmp;
                std::atomic_thread_fence(std::memory_order_release);
                instance_.store(tmp, std::memory_order_relaxed);
        return tmp;

    }




}

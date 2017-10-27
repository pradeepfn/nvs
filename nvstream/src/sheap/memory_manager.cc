
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/filesystem.hpp>
#include <nvs/log.h>

#include "nvs/errorCode.h"
#include "nvs/memory_manager.h"
#include "root.h"

#define NVS_BASE_DIR  "/dev/shm"
#define NVS_USER "unity"

namespace nvs {

/*
 * Internal implemenattion of MemoryManager
 */
    class MemoryManager::Impl_
    {
    public:
        static std::string const rootHeapPath; // path of the heap root

        Impl_()
                : root_heap_(rootHeapPath)
        {
        }

        ~Impl_()
        {
        }

        ErrorCode Init();
        ErrorCode Final();

        void *GlobalToLocal(GlobalPtr ptr);
        GlobalPtr LocalToGlobal(void *addr);

        ErrorCode CreateLog(PoolId id, size_t shelf_size);
        ErrorCode DestroyLog(PoolId id);
        ErrorCode FindLog(PoolId id, Log **log);
        Log *FindLog(PoolId id);


        bool is_ready_;
        RootHeap root_heap_;

    };

    std::string const MemoryManager::Impl_::rootHeapPath = std::string(NVS_BASE_DIR) + "/" + NVS_USER + "_NVS_ROOT";

    ErrorCode MemoryManager::Impl_::Init()
    {

        // create SHELF_BASE_DIR if it does not exist
        boost::filesystem::path nvs_base_path = boost::filesystem::path(NVS_BASE_DIR);
        if (boost::filesystem::exists(nvs_base_path) == false)
        {
            bool ret = boost::filesystem::create_directory(nvs_base_path);
            if (ret == false)
            {
                LOG(fatal) << "NVS: Failed to create SHELF_BASE_DIR " << NVS_BASE_DIR;
                exit(1);
            }
        }
        ErrorCode e_ret = root_heap_.Open();

        if(e_ret == HEAP_NOT_EXIST){
            e_ret = root_heap_.Create();
            if(e_ret != NO_ERROR){
                LOG(fatal) << "NVS: failed to create new heap";
                exit(1);
            }
            // heap creation successful
            e_ret = root_heap_.Open();
            if(e_ret != NO_ERROR){
                LOG(fatal) << "NVS: fail opening heap";
                exit(1);
            }
        }

        is_ready_ = true;
        return NO_ERROR;
    }

    ErrorCode MemoryManager::Impl_::Final()
    {
        ErrorCode ret = root_heap_.Close();
        if (ret!=NO_ERROR)
        {
            LOG(fatal) << "NVS: Root Heap close failed" << rootHeapPath;
            exit(1);
        }

        is_ready_ = false;
        return NO_ERROR;
    }


    ErrorCode MemoryManager::Impl_::CreateLog(PoolId id, size_t size)
    {
        assert(is_ready_ == true);
        assert(id > 0);

        ErrorCode ret = NO_ERROR;

        //TODO: heap root mod lock acquire

        if (root_heap_.isLogExist(id)){
            //TODO: release lock
            LOG(error) << "MemoryManager : the log id (" << (uint64_t)id << "in use";
            return ID_IN_USE;
        }

        // we create a new log
        Log shared_log(id);
        ret = shared_log.Create(size);

        //store the log details on heap-root. pmem transaction
        ret = root_heap_.addLog(id);
        //TODO: heap root mod lock release

        if(ret != NO_ERROR){
            LOG(fatal) << "MemoryManager : error" << ret;
        }

        return ret;
    }

    ErrorCode MemoryManager::Impl_::DestroyLog(PoolId id)
    {

    }

    ErrorCode MemoryManager::Impl_::FindLog(PoolId id, Log **log)
    {

    }

    Log *MemoryManager::Impl_::FindLog(PoolId id)
    {

    }

    void* MemoryManager::Impl_::GlobalToLocal(GlobalPtr ptr) {


    }

    GlobalPtr MemoryManager::Impl_::LocalToGlobal(void *addr)
    {

    }



/*
 * Public APIs of MemoryManager
 */

// thread-safe Singleton pattern with C++11
// see http://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/
    std::atomic<MemoryManager*> MemoryManager::instance_;
    std::mutex MemoryManager::mutex_;
    MemoryManager *MemoryManager::GetInstance()
    {
        MemoryManager *tmp = instance_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            tmp = instance_.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
                tmp = new MemoryManager;
                std::atomic_thread_fence(std::memory_order_release);
                instance_.store(tmp, std::memory_order_relaxed);
            }
        }
        return tmp;
    }

    MemoryManager::MemoryManager()
            : pimpl_{new Impl_}
    {
        ErrorCode ret = pimpl_->Init();
        assert(ret == NO_ERROR);
    }

    MemoryManager::~MemoryManager()
    {
        ErrorCode ret = pimpl_->Final();
        assert(ret == NO_ERROR);
    }


    ErrorCode MemoryManager::CreateLog(PoolId id, size_t size)
    {
        return pimpl_->CreateLog(id, size);
    }

    ErrorCode MemoryManager::DestroyLog(PoolId id)
    {
        return pimpl_->DestroyLog(id);
    }

    ErrorCode MemoryManager::FindLog(PoolId id, Log **log)
    {
        return pimpl_->FindLog(id, log);
    }

    Log *MemoryManager::FindLog(PoolId id)
    {
        return pimpl_->FindLog(id);
    }


    void *MemoryManager::GlobalToLocal(GlobalPtr ptr)
    {
        return pimpl_->GlobalToLocal(ptr);
    }

    GlobalPtr MemoryManager::LocalToGlobal(void *addr)
    {
        return pimpl_->LocalToGlobal(addr);
    }


} // namespace nvs

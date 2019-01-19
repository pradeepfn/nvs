
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/filesystem.hpp>
#include <nvs/log.h>

#include "nvs/errorCode.h"
#include "nvs/memory_manager.h"
#include "root.h"

#define NVS_BASE_DIR  "/dev/shm"
//#define NVS_BASE_DIR  "/mnt/pmfs"
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

        ErrorCode CreateLog(LogId id, size_t shelf_size, Log **log);
        ErrorCode DestroyLog(LogId id);
        ErrorCode FindLog(LogId id, Log **log, size_t);
        //Log *FindLog(LogId id);


        bool is_ready_;
        RootHeap root_heap_;
        std::map<LogId, Log *> idToLogMap;

    };

    std::string const MemoryManager::Impl_::rootHeapPath = std::string(NVS_BASE_DIR) + "/" + NVS_USER + "_NVS_ROOT";

    ErrorCode MemoryManager::Impl_::Init()
    {

        is_ready_ = true;
        return NO_ERROR;
    }

    ErrorCode MemoryManager::Impl_::Final()
    {

        is_ready_ = false;
        return NO_ERROR;
    }


    ErrorCode MemoryManager::Impl_::CreateLog(LogId id, size_t size, Log **log)
    {
        assert(is_ready_);
        assert(id >= 0);

        ErrorCode ret = NO_ERROR;



        if (root_heap_.isLogExist(id)){
            LOG(error) << "MemoryManager : the log id (" << (uint64_t)id << ") in use";
            return ID_IN_USE;
        }
        this->root_heap_.mtx->lock();
        // we create a new log add it to soft state
        std::string logPath = this->rootHeapPath + std::to_string(id);
        *log = new Log(logPath, size ,id,true);
        this->root_heap_.mtx->unlock();

        std::map<LogId, Log *>::iterator it;
        if((it = idToLogMap.find(id)) == this->idToLogMap.end()){
            this->idToLogMap[id] = *log;
        }else{
            LOG(error) << "Log corresponding to PoolID already exists";
        }

        ret = root_heap_.addLog(id);

        if(ret != NO_ERROR){
            LOG(fatal) << "MemoryManager : error" << ret;
        }

        return ret;
    }

    ErrorCode MemoryManager::Impl_::DestroyLog(LogId id)
    {

    }

    ErrorCode MemoryManager::Impl_::FindOrCreateLog(LogId id, Log **log,size_t log_size)
    {
        assert(is_ready_);
        assert(id>=0);

        ErrorCode  ret = NO_ERROR;
        root_heap_.mtx->lock();
        // first look through soft state map
        std::map<LogId , Log *> :: iterator it;
        if((it = this->idToLogMap.find(id)) != this->idToLogMap.end()){
            *log = it->second;
            assert(*log != NULL);
            root_heap_.mtx->unlock();
            return NO_ERROR;
        }else {

            if (!root_heap_.isLogExist(id)) {
                LOG(error) << "MemoryManager : the log id (" << (uint64_t) id <<") not found. Creating new";

                // we create a new log add it to soft state
                std::string logPath = this->rootHeapPath + std::to_string(id);
                *log = new Log(logPath, log_size ,id,true);

                std::map<LogId, Log *>::iterator it;
                if((it = idToLogMap.find(id)) == this->idToLogMap.end()){
                    this->idToLogMap[id] = *log;
                }else{
                    LOG(error) << "Log corresponding to PoolID already exists";
                }

                ret = root_heap_.addLog(id);

                if(ret != NO_ERROR){
                    LOG(fatal) << "MemoryManager : error" << ret;
                }

                root_heap_.mtx->unlock();
                return ret;
            }

            *log = new Log(this->rootHeapPath + std::to_string(id),log_size ,id,false);

            root_heap_.mtx->unlock();
            return NO_ERROR;
        }

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


    ErrorCode MemoryManager::CreateLog(LogId id, size_t size, Log **log)
    {
        return pimpl_->CreateLog(id, size, log);
    }

    ErrorCode MemoryManager::DestroyLog(LogId id)
    {
        return pimpl_->DestroyLog(id);
    }

    ErrorCode MemoryManager::FindOrCreateLog(LogId id, Log **log,size_t log_size)
    {
        return pimpl_->FindOrCreateLog(id, log, log_size);
    }


} // namespace nvs

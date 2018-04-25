
#ifndef NVS_ROOT_HEAP_H
#define NVS_ROOT_HEAP_H

// root structure that hold all the metadata to other logs. This is not based on log structured memory
// lets maintain consistency of this structure with libpmemobj


#include <string>
#include <nvs/log_id.h>
#include <libpmemobj.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "nvs/errorCode.h"

namespace nvs {


    class RootHeap
    {
    public:
        RootHeap() = delete; // no default
        RootHeap(std::string pathname);
        ~RootHeap();

        ErrorCode Create();
        ErrorCode Destroy();
        ErrorCode Open();
        ErrorCode Close();
        bool Exist();
        bool IsOpen();

        ErrorCode addLog(LogId id);
        bool isLogExist(LogId id);

        void *Addr();


        boost::interprocess::interprocess_mutex *mtx; // global lock to sync between processes
    private:
        std::string root_file_path;
        PMEMobjpool *pop;
        boost::interprocess::managed_shared_memory managed_shm;

    };

} // namespace nvs
#endif //NVS_ROOT_HEAP_H

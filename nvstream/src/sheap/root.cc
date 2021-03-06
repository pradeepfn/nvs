#include <libpmemobj/base.h>
#include <libpmemobj.h>
#include <nvs/log.h>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "root.h"
#include "layout.h"


#define NVS_VSHM "nvs_boost_shm"
#define ROOT_MTX "root_mtx"

namespace nvs{


    RootHeap::RootHeap(std::string pathname)
            :root_file_path(pathname),managed_shm(boost::interprocess::open_or_create, NVS_VSHM, 1024)
    {

        this->mtx = managed_shm.find_or_construct<boost::interprocess::interprocess_mutex>(ROOT_MTX)();
        this->mtx->lock();
        this->mtx->unlock();
        if(this->mtx == NULL){
            LOG(error) << "mutex not found on boost shared memory";
        }

    }

    RootHeap::~RootHeap()
    {

    }

     ErrorCode RootHeap::Open() {
           pop =  pmemobj_open(root_file_path.c_str(),
                               POBJ_LAYOUT_NAME(nvstream_store));
           if(pop == NULL){
               LOG(error) << "root.cc : error while opening root heap";
               exit(1);
           }
           return NO_ERROR;
     }


    ErrorCode RootHeap::addLog(LogId id) {

       this->mtx->lock();
       {
           this->Open();
           TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);

           // TODO: persist
           D_RW(root_heap)->log_id[D_RO(root_heap)->length] = id;
           D_RW(root_heap)->length = D_RO(root_heap)->length + 1;
           this->Close();
       }
        this->mtx->unlock();
        return NO_ERROR;

    }

    ErrorCode RootHeap::Destroy() {

    }


    ErrorCode RootHeap::Close() {
        pmemobj_close(pop);
        pop = NULL;
    }

    bool RootHeap::isLogExist(LogId id) {

        this->mtx->lock();
        {
            this->Open();
            TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);

            for (int i = 0; i < D_RO(root_heap)->length; i++) {
                if (D_RO(root_heap)->log_id[i] == id) {
                    this->Close();
                    this->mtx->unlock();
                    return true;
                }
            }
            this->Close();
        }
        this->mtx->unlock();
        return false;
    }
 }

#include <nvs/log.h>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "root.h"
#include "layout.h"


#define NVS_VSHM "nvs_boost_shm"
#define ROOT_MTX "root_mtx"
#define ROOT_SIZE 4096

namespace nvs{


    RootHeap::RootHeap(std::string pathname)
            :root_file_path(pathname),managed_shm(boost::interprocess::open_or_create, NVS_VSHM, 1024)
    {

        this->mtx =
                managed_shm.find_or_construct<boost::interprocess::interprocess_mutex>(ROOT_MTX)();
        this->mtx->lock();
        this->mtx->unlock();
        if(this->mtx == NULL){
            LOG(error) << "mutex not found on boost shared memory";
        }
        this->Open();
    }

    RootHeap::~RootHeap()
    {

    }

     ErrorCode RootHeap::Open() {
         int fd = open(root_file_path.c_str(), O_RDWR, 0600);
         if (fd <0) {
             std::cout << "root file "<< root_file_path.c_str()
                       << "open failed " << strerror(errno);
             return nvs::ErrorCode::OPEN_FAILED;
         }
         this->addr = (char *) mmap(NULL,ROOT_SIZE,PROT_READ| PROT_WRITE,
                     MAP_SHARED,fd,0);
         return NO_ERROR;
     }


    ErrorCode RootHeap::addLog(LogId id) {

       this->mtx->lock();
       {
           struct nvs_root *root = (struct nvs_root *)addr;

           //TODO: transactional update
           root->log_id[root->length] = id;
           root->length = root->length+1;

       }
        this->mtx->unlock();
        return NO_ERROR;

    }

    ErrorCode RootHeap::Destroy() {

    }


    ErrorCode RootHeap::Close() {
        int ret = munmap(this->addr, ROOT_SIZE);
    }

    bool RootHeap::isLogExist(LogId id) {

        //this->mtx->lock();
        {
            struct nvs_root *root = (struct nvs_root *)addr;
            for(int i=0; i < root->length; i++){
                if(root->log_id[i] == id){
                    this->mtx->unlock();
                    return true;
                }
            }
        }
        //this->mtx->unlock();
        return false;
    }
 }

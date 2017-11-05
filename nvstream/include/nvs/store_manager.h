
#ifndef NVSTREAM_STORE_MANAGER_H
#define NVSTREAM_STORE_MANAGER_H

#include "store.h"

namespace nvs{

    class StoreManager
    {
    public:

        static Store *GetInstance(std::string storePath);

    private:
        static std::atomic<Store *> instance_;
        static std::mutex mutex_;

        StoreManager();
        ~StoreManager();
    };


}


#endif //NVSTREAM_STORE_MANAGER_H

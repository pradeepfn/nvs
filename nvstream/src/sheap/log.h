//
// Created by pradeep on 10/25/17.
//

#ifndef NVS_LOG_H
#define NVS_LOG_H

#include <stddef.h>
#include <libpmemlog.h>
#include <nvs/global_ptr.h>
#include "nvs/errorCode.h"
#include "nvs/pool_id.h"

namespace  nvs{

    class RootHeap;

// single-shelf heap
    class Log
    {
    public:
        Log() = delete;
        Log(PoolId pool_id);
        ~Log();

        ErrorCode Create(size_t shelf_size);
        ErrorCode Destroy();
        bool Exist();

        ErrorCode Open();
        ErrorCode Close();
        size_t Size();
        bool IsOpen()
        {
            return is_open_;
        }

        GlobalPtr append (char *data,size_t size);
        void Free (GlobalPtr global_ptr);

        void *GlobalToLocal(GlobalPtr global_ptr);
        // TODO: not implemented yet
        GlobalPtr LocalToGlobal(void *addr);

    private:
        static int const kShelfIdx = 0; // this is the only shelf in the pool

        PoolId pool_id_;
        std::string logPath;
        PMEMlogpool *lp;
        size_t size_;
        RootHeap *rootHeap;
        bool is_open_;
    };

}
#endif //NVS_LOG_H

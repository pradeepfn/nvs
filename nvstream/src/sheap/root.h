
#ifndef NVS_ROOT_HEAP_H
#define NVS_ROOT_HEAP_H

// root structure that hold all the metadata to other logs. This is not based on log structured memory
// lets maintain consistency of this structure with libpmemobj


#include <string>

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
        void *Addr();

    private:
        static size_t const rootSize = 128*1024*1024; // 128MB
        std::string root_file_path;
        struct root_heap *rp;
        PMEMobjpool *pop;
    };

} // namespace nvs
#endif //NVS_ROOT_HEAP_H

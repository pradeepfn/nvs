
#include <nvs/runtimeManager.h>
#include <nvmm/memory_manager.h>
#include <runtime/serializationTypes.h>
#include <runtime/constants.h>
#include "test.h"

#define SHELF_BASE_DIR "/dev/shm/pradeep"
#define SHELF_USER "pradeep"

namespace nvs {

    void initStore(){
        nvmm::MemoryManager *mm = nvmm::MemoryManager::GetInstance();

        // create a new 128MB NVM region with pool id 2
        nvmm::PoolId pool_id = 1;
        size_t size = 1*1024; // 1KB
        nvmm::ErrorCode ret = mm->CreateRegion(pool_id, size);

        // acquire the region
        nvmm::Region *region = NULL;
        ret = mm->FindRegion(pool_id, &region);

        // open and map the region
        ret = region->Open(O_RDWR);
        store_t* st_head = NULL;
        ret = region->Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&st_head);
        assert(ret == NO_ERROR);

        std::string storeName= "testStore";
        // use the region
        st_head->next = Constants::LIST_TERMINATOR;
        snprintf(st_head->storeId, 100, "%s", storeName.c_str());

        // unmap and close the region
        ret = region->Unmap(st_head, size);
        assert(ret == NO_ERROR);
        ret = region->Close();
        assert(ret == NO_ERROR);

        // release the region
        delete region;
    }



    void InitTest(SeverityLevel level, bool to_console) {
        // init boost::log
        if (to_console == true) {
            init_log(level, "");
        } else {
            init_log(level, "mm.log");
        }




        // remove previous files in SHELF_BASE_DIR
        std::string cmd = std::string("exec rm -f ") + SHELF_BASE_DIR + "/" + SHELF_USER + "* > /dev/null";
        system(cmd.c_str());


        // initialize store structures
        initStore();
    }
}
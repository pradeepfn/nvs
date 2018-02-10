
#include <libpmemobj/pool.h>
#include <nvs/errorCode.h>
#include <sheap/layout.h>
#include "test.h"



#define NVS_BASE_DIR  "/dev/shm"
#define NVS_USER "unity"
namespace nvs{


    void create_root(){
        PMEMobjpool *pop;
        std::string root_file_path = "/dev/shm/unity_NVS_ROOT";
        nvs::init_log(nvs::SeverityLevel::all, "");
        pop = pmemobj_open(root_file_path.c_str(),
                           POBJ_LAYOUT_NAME(nvstream_store));
        if (pop != NULL) {
            std::cout << "root heap structures found!. clean them first";
            exit(1);
        }

        pop = pmemobj_create(root_file_path.c_str(),
                             POBJ_LAYOUT_NAME(nvstream_store),
                             PMEMOBJ_MIN_POOL, 0666);
        if (pop == NULL) {
            std::cout << "root.cc : error while creating root heap";
            exit(1);
        }
        //TODO : persist
        TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);
        D_RW(root_heap)->length = 0;
        pmemobj_close(pop);
        std::cout << "successfully created the root heap";
    }


    void InitTest(SeverityLevel level, bool to_console) {
        // init boost::log
        if (to_console == true) {
            nvs::init_log(level, "");
        } else {
            nvs::init_log(level, "mm.log");
        }
        // remove previous files in SHELF_BASE_DIR
        std::string cmd = std::string("exec rm -rf ") + NVS_BASE_DIR + "/" + NVS_USER + "* > /dev/null";
        system(cmd.c_str());
        create_root();
        //create root heap
    }
}
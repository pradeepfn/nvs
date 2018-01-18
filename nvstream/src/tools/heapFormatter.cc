/*
 *
 * create the root heap in a node/machine. This bootstraps the shared heap
 *
 */

#include "nvs/log.h"
#include "nvs/errorCode.h"
#include "sheap/layout.h"

int main(int argc, char *argv[]) {

    PMEMobjpool *pop;
    std::string root_file_path = "/dev/shm/unity_NVS_ROOT";
    nvs::init_log(nvs::SeverityLevel::all, "");
    pop = pmemobj_open(root_file_path.c_str(),
                       POBJ_LAYOUT_NAME(nvstream_store));
    if (pop != NULL) {
        std::cout << "root heap structures found!. clean them first";
        return nvs::ErrorCode::PMEM_ERROR;
    }

    pop = pmemobj_create(root_file_path.c_str(),
                         POBJ_LAYOUT_NAME(nvstream_store),
                         PMEMOBJ_MIN_POOL, 0666);
    if (pop == NULL) {
        std::cout << "root.cc : error while creating root heap";
        return nvs::ErrorCode::PMEM_ERROR;
    }
    //TODO : persist
    TOID(struct nvs_root) root_heap = POBJ_ROOT(pop, struct nvs_root);
            D_RW(root_heap)->length = 0;
    pmemobj_close(pop);
    std::cout << "successfully created the root heap";

    return nvs::ErrorCode::NO_ERROR;
}

#include "test.h"



#define NVS_BASE_DIR  "/dev/shm"
#define NVS_USER "unity"
namespace nvs{

    void InitTest(SeverityLevel level, bool to_console) {
        // init boost::log
        if (to_console == true) {
            nvs::init_log(level, "");
        } else {
            nvs::init_log(level, "mm.log");
        }
        // remove previous files in SHELF_BASE_DIR
        std::string cmd = std::string("exec rm -f ") + NVS_BASE_DIR + "/" + NVS_USER + "* > /dev/null";
        system(cmd.c_str());
    }
}
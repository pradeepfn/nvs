
#include "test.h"

namespace nvs {

    void InitTest(SeverityLevel level, bool to_console) {
        // init boost::log
        if (to_console == true) {
            nvs::init_log(level, "");
        } else {
            nvs::init_log(level, "mm.log");
        }

        // remove previous files in SHELF_BASE_DIR
        //std::string cmd = std::string("exec rm -f ") + SHELF_BASE_DIR + "/" + SHELF_USER + "* > /dev/null";
        //system(cmd.c_str());
    }
}
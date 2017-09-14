
#include "test.h"

namespace nvs {

    void InitTest(SeverityLevel level, bool to_console) {
        // init boost::log
        if (to_console == true) {
            init_log(level, "");
        } else {
            init_log(level, "mm.log");
        }

        //initialize store structures before tests.


        // remove previous files in SHELF_BASE_DIR
        //std::string cmd = std::string("exec rm -f ") + SHELF_BASE_DIR + "/" + SHELF_USER + "* > /dev/null";
        //system(cmd.c_str());
    }
}
#include <gtest/gtest.h>

#include "test.h"

/*
 * testing the runtime manager API
 */

namespace nvs {

    TEST(RuntimeManagerTest, Store) {


    }


    TEST(RuntimeManagerTest, Key) {


    }


} // namespace nvs


int main(int argc, char **argv) {
    InitTest(nvs::fatal, true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}




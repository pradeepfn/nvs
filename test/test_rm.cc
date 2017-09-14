#include <gtest/gtest.h>
#include <nvs/runtimeManager.h>

#include "test.h"

/*
 * testing the runtime manager API
 */

namespace nvs {

    TEST(RuntimeManagerTest, Store) {

        std::string storeId = "testStore";
        Store *myStore = nullptr;

        RuntimeManager *rm = RuntimeManager::getInstance();

        EXPECT_EQ(ID_NOT_FOUND, rm->findStore(storeId,&myStore));
        EXPECT_EQ(NO_ERROR, rm->createStore(storeId,&myStore));

        EXPECT_EQ(NO_ERROR, rm->findStore(storeId,&myStore));

        EXPECT_EQ(NO_ERROR, rm->close());
    }


    TEST(RuntimeManagerTest, Key) {


    }


} // namespace nvs


int main(int argc, char **argv) {
    InitTest(nvs::fatal, true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}




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

        EXPECT_EQ(STORE_NOT_FOUND, rm->findStore(storeId,&myStore));
        EXPECT_EQ(NO_ERROR, rm->createStore(storeId,&myStore));

        EXPECT_EQ(NO_ERROR, rm->findStore(storeId,&myStore));

        EXPECT_EQ(NO_ERROR, rm->close());
    }


    TEST(RuntimeManagerTest, Key) {
        std::string storeId = "testStore";
        Store *myStore = nullptr;

        RuntimeManager *rm = RuntimeManager::getInstance();
        rm->createStore("testKeyStore", &myStore);

        uint64_t *addr;

        EXPECT_EQ(KEY_NOT_EXIST, myStore->get("testKey", 0, &addr));
        EXPECT_EQ(KEY_NOT_EXIST, myStore->put("testKey",1));

        EXPECT_EQ(NO_ERROR, myStore->create_obj("testKey",100,&addr));

        EXPECT_EQ(NO_ERROR, myStore->get("testKey", 0, &addr));
        EXPECT_EQ(NO_ERROR, myStore->put("testKey",1));
    }


} // namespace nvs


int main(int argc, char **argv) {
    InitTest(nvs::fatal, true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}




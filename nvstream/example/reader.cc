#include <string>

#include <nvs/errorCode.h>
#include <common/nanassert.h>
#include "nvs/runtimeManager.h"

#define VERSION 1

using namespace nvs;

int main(int argc, char **argv){


    RuntimeManager *rm = RuntimeManager::getInstance();

    //find the store
    Store *store = nullptr;
    ErrorCode ret = rm->findStore("example_workflow", &store);
    I(ret = NO_ERROR);

    std::string key = "foo";
    //get object metadata  with key,version
    store->get(key,VERSION, nullptr);

    //use object



    store->get(key, VERSION+1, nullptr);


    //use object


    store->close();
    rm->close();

    return 0;


}
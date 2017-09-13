#include <iostream>
#include <string>

#include <common/nanassert.h>
#include <nvs/errorCode.h>
#include "nvs/runtimeManager.h"

#define FOO_SIZE 4000
#define VERSION  1

using namespace nvs;

int main(int argc, char **argv){



    RuntimeManager *rm = RuntimeManager::getInstance();
    Store *myStore;

    //create a new root for the workflow
    ErrorCode ret = rm->createStore("example_workflow",&myStore);
    I(ret == NO_ERROR);

    ret = rm->findStore("example_workflow", &myStore);
    I(ret == NO_ERROR);

    uint64_t *addr;
    //create new object with a key
    myStore->create_obj("foo", FOO_SIZE, &addr);

    //use object



    //create a version/snapshot of the object
    myStore->put("foo", VERSION);


    //use object

    myStore->put("foo", VERSION+1);


    //close store and manager
    myStore->close();
    rm->close();

    return 0;



}
//
// Created by pradeep on 8/28/17.
//

int main(int argc, char **argv){

    init_log(off);

    RuntimeManager *rm = RuntimeManager::GetInstance();

    //create a new root for the workflow
    ErrorCode ret = rm->createStore("example_workflow");
    assert(ret == NO_ERROR);


    //get hold of root
    Store *store = nullptr;
    ret = rm->findStore("example_workflow", &store);
    assert(ret == NO_ERROR);

    //create new object with a key
    store->create_obj("foo", FOO_SIZE,);

    //use object



    //create a version/snapshot of the object
    store->put("foo", VERSION);


    //use object

    stroe->put("foo", VERSION+1);


    //close store and manager
    store->close();
    rm->close();

    return 0;



}
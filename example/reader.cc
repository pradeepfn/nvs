//
// Created by pradeep on 8/28/17.
//

int main(int argc, char **argv){

    init_log(off);

    RuntimeManager *rm = RuntimeManager::GetInstance();

    //find the store
    Store *store = nullptr;
    ret = rm->findStore("example_workflow", &store);
    assert(ret = NO_ERROR);

    //get object metadata  with key,version
    store->get(key,VERSION);

    //use object



    store->get(key, VERSION+1);


    //use object


    store->close();
    rm->close();

    return 0;


}
//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <nvs/runtimeManager.h>
#include <common/nanassert.h>
#include <nvmm/error_code.h>
#include <nvmm/global_ptr.h>
#include "serializationTypes.h"
#include "gtstore.h"

namespace nvs{

    GTStore::GTStore(store_t *st){

        // store structure has the id to metadata heap
        Heap *heap = NULL;
        mm->FindHeap(st->meta_heap, &heap);

        nvmm::ErrorCode ret = heap->Open();
        I(ret == NO_ERROR);




    }

    GTStore::GTStore(uint64_t *addr, std::string storeId):key_head((objkey_t *)addr),storeId(storeId)
    {
        srl_store = (store_t *) addr;
        I(GTStoreId.compare(std:string(srl_GTStore->GTStoreId)));



        /* we create a region to GTStore keys for this GTStore */
        //kr_addr = rt->findKeyRegion(GTStoreId);
    }

    GTStore::~GTStore() {}

    /* traverse the list of key structs and create a new key if not found*/
    ErrorCode GTStore::create_obj(std::string key, uint64_t size, uint64_t **obj_addr)
    {
        //create an key structure in shared heap and populate
        nvmm::GlobalPtr ptr=  this->heap->Alloc(sizeof(key_t));
        objkey_t *obj = (objkey_t*)this->mm->GlobalToLocal(ptr);
        obj->keyId =
        obj->obj_addr =
        this->keyList->addNode();
        return NO_ERROR;
    }


    /*
     * this is similar to a commit operation. we do not need the object
     * address and size for put operation as the runtime already know that
     * detail.
     */
    ErrorCode GTStore::put(std::string key, uint64_t version)
    {

            // first traverse the map and find the key object

            objkey_t *obj;
            // if not traverse the shared memory object structures
            if(!this->keyList->findNode(key,&obj){
            // new key object added to map, then create a new version


            }
            return ELEM_NOT_FOUND;
    }

    /*
     *  the object address is redundant here
     */
    ErrorCode GTStore::get(std::string key, uint64_t version, uint64_t **obj_addr)
    {

        // first traverse the map and find the key object




    }

    ErrorCode GTStore::close() {}


    Key * GTStore::findKey(std::string key) {



    }

    objkey_t * GTStore::lptr(uint64_t offset) {
            return (objkey_t *)(key_head + offset);
        }
}

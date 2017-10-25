//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <nvs/runtimeManager.h>
#include <common/nanassert.h>
#include "serializationTypes.h"
#include "nvs_store.h"

namespace nvs{

    NVSStore::NVSStore(store_t *st){

        // store structure has the id to metadata heap
        Heap *heap = NULL;
        mm->FindHeap(st->meta_heap, &heap);

        nvmm::ErrorCode ret = heap->Open();
        I(ret == NO_ERROR);




    }

    NVSStore::NVSStore(uint64_t *addr, std::string storeId):key_head((objkey_t *)addr),storeId(storeId)
    {
        srl_store = (store_t *) addr;
        I(NVSStoreId.compare(std:string(srl_NVSStore->NVSStoreId)));



        /* we create a region to NVSStore keys for this NVSStore */
        //kr_addr = rt->findKeyRegion(NVSStoreId);
    }

    NVSStore::~NVSStore() {}

    /* traverse the list of key structs and create a new key if not found*/
    ErrorCode NVSStore::create_obj(std::string key, uint64_t size, uint64_t **obj_addr)
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
    ErrorCode NVSStore::put(std::string key, uint64_t version)
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
    ErrorCode NVSStore::get(std::string key, uint64_t version, uint64_t **obj_addr)
    {

        // first traverse the map and find the key object




    }

    ErrorCode NVSStore::close() {}


    Key * NVSStore::findKey(std::string key) {



    }

    objkey_t * NVSStore::lptr(uint64_t offset) {
            return (objkey_t *)(key_head + offset);
        }
}

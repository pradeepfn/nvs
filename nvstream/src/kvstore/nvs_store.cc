//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <nvs/runtimeManager.h>
#include "nvs/log.h"
#include "nvs_store.h"

namespace nvs{


    NVSStore::NVSStore(std::string storeId, ProcessId pid):
            storeId(storeId),pid(pid)
    {

    }

    NVSStore::~NVSStore() {}

    ErrorCode NVSStore::create_obj(std::string key, uint64_t size, uint64_t **obj_addr)
    {

        void *tmp_ptr = malloc(size);
        Object *obj = new Object(key,size,0,tmp_ptr);
        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
            objectMap[key] = obj;
        }else{
            LOG(fatal) << "not implemented yet";
            exit(1);
        }
        *obj_addr = (uint64_t *)tmp_ptr;

        return NO_ERROR;
    }


    /*
     * this is similar to a commit operation. we do not need the object
     * address and size for put operation as the kvstore already know that
     * detail.
     */
    ErrorCode NVSStore::put(std::string key, uint64_t version)
    {

            std::map<std::string, Object *>::iterator it;
            // first traverse the map and find the key object
            if((it = objectMap.find(key)) != objectMap.end()){
                Object *obj = it->second;
                uint64_t version = obj->getVersion();
                // TODO: write to log

                //increase the soft state
                obj->setVersion(version+1);
                return NO_ERROR;
            }
            LOG(error) << "element not found";
            return ELEM_NOT_FOUND;
    }

    /*
     *  the object address is redundant here
     */
    ErrorCode NVSStore::get(std::string key, uint64_t version, uint64_t **obj_addr)
    {

       /* 1. get hold of the read lock for the key
          2. Lookup key and version - the returned object contains the metadata to
          construct the data
          3. copy data in to return object
          4. given object can be null or already returned object address.

        */

    }


    Key * NVSStore::findKey(std::string key) {

    }

}

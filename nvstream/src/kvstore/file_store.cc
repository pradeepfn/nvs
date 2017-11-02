#include <nvs/log.h>
#include "file_store.h"

namespace nvs{


    ErrorCode FileStore::put(std::string key, uint64_t version) {

    }


    ErrorCode FileStore::get(std::string key, uint64_t version, uint64_t **addr) {

    }

    ErrorCode FileStore::create_obj(std::string key, uint64_t size, uint64_t **obj_addr) {

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







}

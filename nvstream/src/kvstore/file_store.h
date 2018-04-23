//
// Created by pradeep on 11/2/17.
//

#ifndef NVSTREAM_FILE_STORE_H
#define NVSTREAM_FILE_STORE_H

#include <map>
#include <nvs/store.h>
#include "object.h"

#if defined(_TMPFS)
    #define ROOT_FILE_PATH "/dev/shm/unity"
#elif defined(_PMFS)
    #define ROOT_FILE_PATH "/mnt/pmfs/unity"
#else
#define ROOT_FILE_PATH "INVALID"
#endif
/*
 *
 * I/O happens through file API. New file is created for each version
 *
 */

namespace nvs{
    class FileStore: public Store{

    public:

        FileStore(std::string storeId);

        ~FileStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode put(std::string key, uint64_t version);

        ErrorCode put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        void stats();


    private:
        std::map<std::string, Object *> objectMap;
        std::string storeId;

    };
}


#endif //NVSTREAM_FILE_STORE_H

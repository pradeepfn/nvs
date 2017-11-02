//
// Created by pradeep on 11/2/17.
//

#ifndef NVSTREAM_FILE_STORE_H
#define NVSTREAM_FILE_STORE_H


#include <nvs/store.h>
#include "object.h"

#define FILE_PATH "/dev/shm"

/*
 *
 * I/O happens through fiel API. New file is created for each version
 *
 */

namespace nvs{
    class FileStore: public Store{

    public:

        ErrorCode create_obj(std::string key, uint64_t size, uint64_t **obj_addr);

        ErrorCode put(std::string key, uint64_t version);

        ErrorCode get(std::string key, uint64_t version, uint64_t **obj_addr);


    private:
        std::map<std::string, Object *> objectMap;

    };
}


#endif //NVSTREAM_FILE_STORE_H

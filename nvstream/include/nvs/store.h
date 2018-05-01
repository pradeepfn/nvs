//
// Created by pradeep on 9/14/17.
//

#ifndef YUMA_STORE_H
#define YUMA_STORE_H

#include <cstdint>
#include <string>

#include "errorCode.h"

namespace nvs {
    class Store {
    public:

        ~Store(){};

        virtual ErrorCode create_obj(std::string key, uint64_t size, void **addr) = 0;

        virtual ErrorCode free_obj(void *obj_addr) =0;

        virtual uint64_t put(std::string key, uint64_t version)=0;

        virtual uint64_t put_all()=0;

        virtual ErrorCode get(std::string key, uint64_t version, void *addr)=0;

        /* returns object. Runtime allocates the memory for returned object */
        virtual ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr) = 0;

        virtual std::string get_store_id() = 0;

        virtual void stats() = 0;

    };

}
#endif //YUMA_STORE_H

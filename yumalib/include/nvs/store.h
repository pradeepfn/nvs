//
// Created by pradeep on 9/14/17.
//

#ifndef YUMA_STORE_H
#define YUMA_STORE_H

namespace nvs {
    class Store {
    public:

        ~Store(){};

        virtual ErrorCode create_obj(std::string key, uint64_t size, uint64_t **addr) = 0;

        virtual ErrorCode put(std::string key, uint64_t version)=0;

        virtual ErrorCode get(std::string key, uint64_t version, uint64_t **addr)=0;

        virtual ErrorCode close() = 0;
    };

}
#endif //YUMA_STORE_H

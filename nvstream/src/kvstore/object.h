//
// Created by pradeep on 10/29/17.
//

#ifndef NVSTREAM_OBJECT_H
#define NVSTREAM_OBJECT_H

namespace nvs {
    class Object {

    public:
        Object(std::string name, uint64_t size, uint64_t version, void *ptr){

        }
        ~Object();




    private:
        std::string name;
        uint64_t size;
        uint64_t version;
        void *ptr;
    };
}

#endif //NVSTREAM_OBJECT_H

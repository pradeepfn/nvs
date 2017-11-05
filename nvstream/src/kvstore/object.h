//
// Created by pradeep on 10/29/17.
//

#ifndef NVSTREAM_OBJECT_H
#define NVSTREAM_OBJECT_H

#include <cstdint>
#include <string>

namespace nvs {
    class Object {

    public:
        Object(std::string name, uint64_t size, uint64_t version, void *ptr);
        ~Object();

        uint64_t getVersion(){
            return version;
        }
        uint64_t getSize(){
            return size;
        }
        void * getPtr(){
            return ptr;
        }

        std::string getName(){
            return name;
        }

        void setVersion(uint64_t version);


    private:
        std::string name;
        uint64_t size;
        uint64_t version;
        void *ptr;
    };
}

#endif //NVSTREAM_OBJECT_H

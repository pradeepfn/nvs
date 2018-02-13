//
// Created by pradeep on 10/29/17.
//

#ifndef NVSTREAM_OBJECT_H
#define NVSTREAM_OBJECT_H

#include <cstdint>
#include <string>
#include <vector>


#define PAGE_SIZE 4096

namespace nvs {

#ifdef _DELTA_STORE
    /* structure to keep track of delta chunks */
    struct delta_t{
        uint64_t start_offset;
        uint64_t len;
    };
#endif



    class Object {

    public:
        Object(std::string name, uint64_t size,
               uint64_t version, void *ptr);
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

#ifdef _DELTA_STORE
        uint64_t get_aligned_size(){
            return aligned_size;
        }
        std::vector<struct delta_t> get_delta_chunks();
#endif



        void setVersion(uint64_t version);


    private:
        std::string name;
        uint64_t size;
        uint64_t version;
        void *ptr;
#ifdef _DELTA_STORE
        //bit vector to keep track of modified pages
        std::vector<bool> *bitset;
        uint64_t aligned_size;
#endif // _DELTA_STORE
    };
}

#endif //NVSTREAM_OBJECT_H

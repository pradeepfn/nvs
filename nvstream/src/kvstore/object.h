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


    /* structure to keep track of delta chunks */
    struct delta_t{
        uint64_t start_offset;
        uint64_t len;

        bool operator <(const delta_t& rhs) const{
            return len < rhs.len;
        }

    };




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


        uint64_t get_aligned_size(){
            return aligned_size;
        }


        void set_modified_bit(int64_t page_index){
            if(page_index>0 && page_index < bitset.size()) {
                bitset[page_index] = true;
            }
        }

        std::vector<struct delta_t> get_delta_chunks();


        void reset_bit_vector(){
            std::vector<bool>::iterator it;
            for(it=bitset.begin();it != bitset.end();it++){
                *it = false;
            }
        }




        void setVersion(uint64_t version);


    private:
        std::string name;
        uint64_t size;
        uint64_t version;
        void *ptr;

        //bit vector to keep track of modified pages
        std::vector<bool> bitset;
        uint64_t aligned_size;

    };
}

#endif //NVSTREAM_OBJECT_H




#include <algorithm>
#include "object.h"


namespace nvs{

    Object::Object(std::string name, uint64_t size, uint64_t version, void *ptr):
        name(name),size(size),version(version),ptr(ptr)
    {
#ifdef _DELTA_STORE
        this->aligned_size = (size + PAGE_SIZE-1) & ~(PAGE_SIZE);
        this->bitset.resize(aligned_size/PAGE_SIZE);
#endif
    }

    void Object::setVersion(uint64_t version) {
        this->version = version;
    }

    std::vector<struct delta_t> Object::get_delta_chunks(){

        std::vector<struct delta_t> dcvector; // should maintain in a max/min heap with depth 5
        bool hole = false;
        uint64_t start_offset;
        uint64_t len;

        for(uint64_t i = 0; i < this->bitset.size(); i++){
            if(!hole && !bitset[i]){
                hole = true;
                start_offset = i;
                len++;
            }else if(hole && bitset[i]){
                hole=false;
                struct delta_t dchunk;
                dchunk.start_offset = start_offset;
                dchunk.len = len;
                start_offset = 0;
                len = 0;
                dcvector.push_back(dchunk); /* copy constructor */
            }
        }
        // sort the vector and truncate after max 5
        std::sort(dcvector.begin(), dcvector.end()); // TODO: need comparator
        //truncate the chunk vector
        dcvector.resize(5);
        return dcvector;
    }

    Object::~Object() {

    }

}

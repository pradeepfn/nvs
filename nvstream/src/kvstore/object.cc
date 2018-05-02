


#include <algorithm>
#include "object.h"
#include "nvs/log.h"


namespace nvs{

    Object::Object(std::string name, uint64_t size, uint64_t version, void *ptr):
        name(name),size(size),version(version),ptr(ptr)
    {

#if defined(_DELTA_STORE)
       this->aligned_size = (size + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
       this->bitset.resize(aligned_size/PAGE_SIZE);
	   for(int64_t i = 0; i< this->bitset.size();i++){
		bitset[i] = 1;
	   }
#else
        this->aligned_size = 0;
#endif

#if defined(_DELTA_STORE)
        LOG(debug)<< "object aligned size , PAGE_SIZE : " << aligned_size << "," <<PAGE_SIZE;
#else
        LOG(debug)<< "object size , PAGE_SIZE : " << size << "," <<PAGE_SIZE;
#endif

    }

    void Object::setVersion(uint64_t version) {
        this->version = version;
    }

#define WRITTEN_REGION 1
    std::vector<struct delta_t> Object::get_delta_chunks(){

        std::vector<struct delta_t> dcvector;
        bool status = !WRITTEN_REGION;
        uint64_t start_offset;
        uint64_t len;

        for(uint64_t i = 0; i < this->bitset.size(); i++){
        	if(bitset[i]){ // if the page is modified
        		if(status == !WRITTEN_REGION){
        			status = WRITTEN_REGION;
        			start_offset = i;
        			len=1;
        		}else{
        			//we are passing through written region
        			len++;
        		}
        	}
        	/* transitioning from written region to non-modified page, or end of the object */
        	if(!bitset[i] || i == this->bitset.size()-1){
        		if(status == WRITTEN_REGION){
        			status = !WRITTEN_REGION;
        			struct delta_t dchunk;
        			dchunk.start_offset = start_offset*PAGE_SIZE;
        			if(!bitset[i])
        				dchunk.len = len*PAGE_SIZE;
        			else if(i == this->bitset.size()-1)
        				dchunk.len = this->size - dchunk.start_offset; // we only copy to end of actual data
        			dcvector.push_back(dchunk);
        		}
        	}
        }

        LOG(debug) << "Delta list size :  " <<  dcvector.size();
        return dcvector;
    }

    Object::~Object() {

    }

}

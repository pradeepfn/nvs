
#ifndef NVSTREAM_TIMING_STORE_H
#define NVSTREAM_TIMING_STORE_H

#include <common/util.h>
#include <nvs/log.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <fstream>


#define MEGA 1000000

namespace nvs{
    class TimingStore: public Store{

    public:

        TimingStore(Store *st);

        ~TimingStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode free_obj(void *obj_addr);

        uint64_t put(std::string key, uint64_t version);

        uint64_t put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        std::string get_store_id(){
        	LOG(debug) << "not supported";
        	return nullptr;
        }

        void stats();

    private:
        Store *store;
        uint64_t get_start;
        uint64_t get_end;
        uint64_t get_total;
        uint64_t get_niterations;

        uint64_t put_start;
        uint64_t put_end;
        uint64_t put_total;
        uint64_t put_niterations;

        uint64_t putall_start;
        uint64_t putall_end;
        uint64_t putall_total;
        uint64_t putall_niterations;

        uint64_t app_start;
        uint64_t snap_time;
        uint64_t app_end;

        std::map<std::string, uint64_t > vmap; // per variable read times
        std::map<std::string, uint64_t > nmap;

        std::vector<uint64_t> plist; // time for snapshotting in each iteration
        std::vector<uint64_t> tlist; // iteration time list
        std::vector<uint64_t> slist; // snapshot size in each iteration

        uint64_t total_size;

    };


    TimingStore::TimingStore(Store *st):store(st) {
        get_niterations = 0;
        get_total = 0;
        put_niterations = 0;
        put_total=0;
        putall_total=0;
        putall_niterations = 0;
        total_size=0;

        snap_time = 0;
    }

    ErrorCode TimingStore::get(std::string key, uint64_t version, void *obj_addr) {
        ErrorCode ret;
        ret = this->store->get(key,version,obj_addr);
        return ret;
    }


    ErrorCode TimingStore::get_with_malloc(std::string key, uint64_t version, void **addr) {
        ErrorCode ret;
        boost::trim_right(key);
        ret = this->store->get_with_malloc(key,version,addr);
        return ret;
    }


    ErrorCode TimingStore::create_obj(std::string key, uint64_t size, void **obj_addr) {
        ErrorCode ret;
        boost::trim_right(key);
        ret = this->store->create_obj(key,size,obj_addr);
        return ret;
    }

    ErrorCode TimingStore::free_obj(void *obj_addr){
    	ErrorCode ret;
    	ret = this->store->free_obj(obj_addr);
    	return ret;
    }

    uint64_t TimingStore::put(std::string key, uint64_t version) {
        uint64_t ret;
        ret = this->store->put(key,version);
        return ret;
    }


    uint64_t TimingStore::put_all() {
        uint64_t ret;
        ret = this->store->put_all();
        return ret;
    }

    void TimingStore::stats() {
    	std::cout << "stats not available" << std::endl;	
    }

    TimingStore::~TimingStore() {

    }

}

#endif //NVSTREAM_TIMING_STORE_H

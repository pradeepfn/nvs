
#ifndef NVSTREAM_TIMING_STORE_H
#define NVSTREAM_TIMING_STORE_H

#include <common/util.h>
#include <nvs/log.h>
#include <boost/algorithm/string.hpp>


#define MEGA 1000000

namespace nvs{
    class TimingStore: public Store{

    public:

        TimingStore(Store *st);

        ~TimingStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode put(std::string key, uint64_t version);

        ErrorCode put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

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

        std::map<std::string, uint64_t > vmap; // per variable read times
        std::map<std::string, uint64_t > nmap;

        uint64_t total_size;

    };


    TimingStore::TimingStore(Store *st):store(st) {
        get_niterations = 0;
        get_total = 0;
        put_niterations = 0;
        put_total=0;
        total_size=0;
    }

    ErrorCode TimingStore::get(std::string key, uint64_t version, void *obj_addr) {
        ErrorCode ret;
        get_start = read_tsc();
        ret = this->store->get(key,version,obj_addr);
        get_end = read_tsc();
        get_total += (get_end - get_start);
        get_niterations++;
        return ret;
    }


    ErrorCode TimingStore::get_with_malloc(std::string key, uint64_t version, void **addr) {
        ErrorCode ret;
        boost::trim_right(key);
        get_start = read_tsc();
        ret = this->store->get_with_malloc(key,version,addr);
        get_end = read_tsc();
        uint64_t  get_tmp = (get_end - get_start);

        std::map<std::string, uint64_t>::iterator it;

        if((it = vmap.find(key)) != vmap.end()){
            vmap[key] = vmap[key] + get_tmp;
            nmap[key] = nmap[key] + 1;
        }else{
            vmap[key] = get_tmp;
            nmap[key] = 1;
        }
        return ret;
    }


    ErrorCode TimingStore::create_obj(std::string key, uint64_t size, void **obj_addr) {
        ErrorCode ret;

        ret = this->store->create_obj(key,size,obj_addr);
        total_size += size;
        return ret;
    }

    ErrorCode TimingStore::put(std::string key, uint64_t version) {
        ErrorCode ret;
        put_start = read_tsc();
        ret = this->store->put(key,version);
        put_end = read_tsc();
        put_total += (put_end - put_start);
        put_niterations++;
        return ret;
    }


    ErrorCode TimingStore::put_all() {
        ErrorCode ret;
        putall_start = read_tsc();
        ret = this->store->put_all();
        putall_end = read_tsc();
        putall_total += (putall_end - putall_start);
        putall_niterations++;
        return ret;
    }

    void TimingStore::stats() {
        float  ave_get=0, ave_put=0, ave_putall=0;

        if(get_niterations){ ave_get = (float)get_total * MEGA/ (get_niterations * get_cpu_freq());}
        if(put_niterations){ ave_put = (float)put_total * MEGA/ (put_niterations * get_cpu_freq());}
        if(putall_niterations){ ave_putall = (float)putall_total * MEGA/ (putall_niterations * get_cpu_freq());}

        std::cout << std::endl;
        std::cout << "average get time (micro-sec) : " << ave_get << std::endl;
        std::cout << "average put time (micro-sec) : " << ave_put << std::endl;
        std::cout << "average put_all time (micro-sec) : " << ave_putall << std::endl;
        std::cout << "snapshot size (MB) : " << ((float)total_size/(1024*1024)) << std::endl;

        std::cout << "per variable stats"<< std::endl;
        std::map<std::string, uint64_t >::iterator iterator1;
        std::map<std::string, uint64_t >::iterator iterator2;
        for(iterator1 = vmap.begin(), iterator2=nmap.begin(); iterator1!= vmap.end() && iterator2 != nmap.end();
                iterator1++, iterator2++){
            if(iterator1->first != iterator2->first){
                LOG(error) << "mismatch in map elements";
                exit(-1);
            }

            std::cout << iterator1->first + " : "
                      << (float(iterator1->second) * MEGA / (iterator2->second * get_cpu_freq()))
                      << std::endl;
        }

    }

    TimingStore::~TimingStore() {

    }

}

#endif //NVSTREAM_TIMING_STORE_H

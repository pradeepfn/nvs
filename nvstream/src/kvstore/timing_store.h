
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
        app_start = read_tsc();
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
        boost::trim_right(key);
        ret = this->store->create_obj(key,size,obj_addr);
        total_size += size;
        return ret;
    }

    ErrorCode TimingStore::free_obj(void *obj_addr){
    	ErrorCode ret;
    	ret = this->store->free_obj(obj_addr);
    	return ret;
    }

    uint64_t TimingStore::put(std::string key, uint64_t version) {
        uint64_t ret;
        put_start = read_tsc();
        ret = this->store->put(key,version);
        put_end = read_tsc();
        put_total += (put_end - put_start);
        put_niterations++;
        return ret;
    }


    uint64_t TimingStore::put_all() {
        uint64_t ret;
        putall_start = read_tsc();
        ret = this->store->put_all();
        putall_end = read_tsc();
        uint64_t temp = (putall_end - putall_start);
        uint64_t iter_time;
        if(snap_time == 0){ // first snapshot
        	iter_time = putall_end - app_start;
        	snap_time = putall_end;
        }else{
        	iter_time = putall_end - snap_time;
        	snap_time = putall_end;
        }

        plist.push_back(temp);
        slist.push_back(ret);
        tlist.push_back(iter_time);
        putall_total += temp;
        putall_niterations++;
        return ret;
    }



    void TimingStore::stats() {
    	app_end = read_tsc();
    	//std::ostream &ostr = std::cout;


    	std::string delimeter = "/";
    	std::string storeId = this->store->get_store_id();
    	std::string logIdStr = storeId.substr(storeId.find(delimeter) + delimeter.length());
    	uint64_t logId = std::stoi(logIdStr);
    	std::string fname = "stats/store_" + std::to_string(logId) + ".txt";
    	//std::string fname = "stats/store_.txt";
    	std::ofstream file;
    	file.open(fname, std::ios::out|std::ios::trunc);
    	std::ostream &ostr = file;

        float  ave_get=0, ave_put=0, ave_putall=0;

        if(get_niterations){ ave_get = ((float)get_total * MEGA)/ (get_niterations * get_cpu_freq());}
        if(put_niterations){ ave_put = ((float)put_total * MEGA)/ (put_niterations * get_cpu_freq());}
        if(putall_niterations){ ave_putall = ((float)putall_total * MEGA)/ (putall_niterations * get_cpu_freq());}

        ostr << std::endl;
        ostr << "cpu freq : " + std::to_string(get_cpu_freq()) << std::endl;
        ostr << "putall total  : " + std::to_string(putall_total) << std::endl;
        ostr << "putall niterations : " + std::to_string(putall_niterations) << std::endl;
        ostr << std::endl;
        ostr << "average get time (micro-sec) : " << ave_get << std::endl;
        ostr << "average put time (micro-sec) : " << ave_put << std::endl;
        ostr << "average put_all time (micro-sec) : " << ave_putall << std::endl;
        ostr << "snapshot size (MB) : " << ((float)total_size/(1024*1024)) << std::endl;

        ostr << "per iteration stats" << std::endl;
        ostr << "iteration snapshot time (micro-sec) : ";
        std::vector<uint64_t>::iterator list_it;
        for(list_it = plist.begin(); list_it != plist.end();list_it++){
        		ostr << std::to_string( ((float)(*list_it)*MEGA)/get_cpu_freq()) << " ";
        }
        ostr << std::endl;

        ostr << "iteration snapshot size (MB) : ";
        std::vector<uint64_t>::iterator it;
        for(it = slist.begin(); it != slist.end(); it++){
        	ostr << std::to_string(((float)(*it))/(1024*1024)) << " ";
        }
        ostr << std::endl;

        ostr << "iteration time (mirco-sec) : ";
        std::vector<uint64_t>::iterator t_it;
        for(t_it=tlist.begin(); t_it != tlist.end(); t_it++){
        	ostr<<std::to_string(((float)(*t_it)*MEGA)/get_cpu_freq()) << " ";
        }
        ostr<<std::endl;

        ostr<< "total application time (milli-sec) : " << std::to_string(((float)(app_end-app_start)*1000)/get_cpu_freq())
        <<std::endl;

        ostr << "per variable stats"<< std::endl;
        std::map<std::string, uint64_t >::iterator iterator1;
        std::map<std::string, uint64_t >::iterator iterator2;
        for(iterator1 = vmap.begin(), iterator2=nmap.begin(); iterator1!= vmap.end() && iterator2 != nmap.end();
                iterator1++, iterator2++){
            if(iterator1->first != iterator2->first){
                LOG(error) << "mismatch in map elements";
                exit(-1);
            }

            ostr << iterator1->first + " : "
                      << (float(iterator1->second) * MEGA / (iterator2->second * get_cpu_freq()))
                      << std::endl;
        }




    }

    TimingStore::~TimingStore() {

    }

}

#endif //NVSTREAM_TIMING_STORE_H

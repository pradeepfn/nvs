
#include <nvs/memory_manager.h>
#include <map>
#include "nvs/store.h"
#include "constants.h"
#include "object.h"
#include "common/util.h"
#include "libpmem.h"

#ifndef NVS_STORE_H
#define NVS_STORE_H


#define PLOG_SIZE 1024LLU // 2 GB of log space per process
#define NCOMPACTOR_THREADS 2
#define IS_COMPACTOR 0

namespace nvs {

    class NVSStore : public Store {
    private:

        std::string storeId;
        ProcessId pid;
        MemoryManager *mm; // memory manager of the metadata heap
        Log *log; // process local log
        nvs_config_t *config;
        std::map<std::string, Object *> objectMap; // runtime representation of volatile object
        std::map<uint64_t,std::string> addrMap; // address to object mapping. we use this for free


    protected:
    public:
        NVSStore(std::string storeId);

        ~NVSStore();

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr);

        ErrorCode free_obj(void *obj_addr);

        uint64_t put(std::string key, uint64_t version);

        uint64_t put_all();

        ErrorCode get(std::string key, uint64_t version, void *obj_addr);

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr);

        std::string get_store_id(){
        	return storeId;
        }

        void stats();

    };

#define NCHUNKS 4
    static int log_compactor(volatile char *pmemaddr){
        struct lhdr_t *pmem_hdr = (struct lhdr_t *)pmemaddr;


        uint64_t vtail = pmem_hdr->tail;
        uint64_t vhead = pmem_hdr->head;
        uint64_t vwrap_end = pmem_hdr->wrap_end;

        LOG(debug) << "Compacting log, head : " << std::to_string(vhead)
                   <<" tail : " << std::to_string(vtail)
                   << " wrap-end : " << std::to_string(vwrap_end) << " from persistent log";

        uint64_t tmp_offset = vhead;
        uint64_t nchunks = NCHUNKS;
        struct lehdr_t *vlehdr;
        while(tmp_offset != vtail && nchunks > 0){
            //next log header
            tmp_offset = tmp_offset + (sizeof(struct lehdr_t) + ((struct lehdr_t*)(pmemaddr+tmp_offset))->len);
            if(tmp_offset == vwrap_end){
                tmp_offset = sizeof(struct lhdr_t);
            }
            nchunks --;
        }
        vlehdr = ((struct lehdr_t*)(pmemaddr+tmp_offset));
        LOG(debug) << "New head entry, key-name : " << vlehdr->kname
                   << " version : " << std::to_string(vlehdr->version);
        pmem_memcpy_nodrain((void *) &(pmem_hdr->head), &tmp_offset, sizeof(uint64_t));
        pmem_drain();

        printf("log compactor ran\n");
    }


}

#endif //YUMA_STORE_H

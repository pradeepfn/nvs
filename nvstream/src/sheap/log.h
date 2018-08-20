//
// Created by pradeep on 10/25/17.
//

#ifndef NVS_LOG_H
#define NVS_LOG_H

#include <stddef.h>
#include <libpmemlog.h>
#include <nvs/global_ptr.h>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <common/util.h>
#include "nvs/errorCode.h"
#include "nvs/log_id.h"



#define WORD_LENGTH 8
#define COMMIT_FLAG  123456
#define MAGIC_NUMBER 555555

#define KEY_LEN 20

enum HdrType {
    single =0,
    multiple,
    delta,

};



/* log header at the start of the log*/
struct lhdr_t{

    uint64_t magic_number;  // error detection
    uint64_t len;
    volatile uint64_t wrap_end; // wrap around start here.
    volatile uint64_t head; // consume stars from head
    volatile uint64_t tail; // producer appends to tail

};



struct ledelta_t{
    uint64_t start_offset;
    uint64_t len;
};



/* log entry header */
struct lehdr_t{
    char kname[20];
    uint64_t version; // only if a single data item
    uint64_t len; // entry lenth
    HdrType type; // entry type

    uint64_t delta_len;
    struct ledelta_t deltas[5];

};

//structure for log traversing
struct walkentry {
    size_t len;
    void *datap;
    uint64_t version;
    uint64_t start_offset;
    char key[KEY_LEN];
    nvs::ErrorCode err;

    uint64_t delta_len;
    struct ledelta_t deltas[5];


};


static inline void asm_mfence(void)
{
    __asm__ __volatile__ ("mfence");
}

namespace  nvs{

    class RootHeap;

// single-shelf heap
    class Log
    {
    public:
        Log() = delete;
        Log(std::string logpath,uint64_t log_size,LogId log_id);
        ~Log();

        ErrorCode appendv(struct iovec *iovp, int iovcnt);
        ErrorCode appendmv(struct iovec **iovpp, int *iovcnt, int iovpcnt);
        ErrorCode walk(int (*process_chunk)(const void *buf, size_t len, void *arg),void *arg);
        ErrorCode printLog(); // debug purposes

        void Free (GlobalPtr global_ptr);

    private:

        char* to_addr(uint64_t offset);
        void persist();

        LogId log_id;
        std::string logPath;
        char *pmemaddr;
        size_t mapped_len;
        struct lhdr_t *pmem_hdr;

        size_t log_size;
        uint64_t start_offset; /* local volatile copy of pmem queue head */
        uint64_t log_end;
        uint64_t write_offset;
        RootHeap *rootHeap;

        boost::interprocess::managed_shared_memory managed_shm;
        boost::interprocess::interprocess_mutex *mtx; // per log mutex
        threadpool_t *log_compactor;

        void (*compactor)(); // function pointer to the compactor logic


        void compact(uint64_t,uint64_t,uint64_t);
        int next_append(uint64_t,uint64_t *);

    };

}
#endif //NVS_LOG_H

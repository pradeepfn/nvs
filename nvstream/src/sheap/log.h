//
// Created by pradeep on 10/25/17.
//

#ifndef NVS_LOG_H
#define NVS_LOG_H

#include <stddef.h>
#include <libpmemlog.h>
#include <nvs/global_ptr.h>
#include "nvs/errorCode.h"
#include "nvs/log_id.h"



#define WORD_LENGTH 8
#define COMMIT_FLAG -123456

#define KEY_LEN 20

enum HdrType {
    single =0,
    multiple,
    delta
};



/* log header at the start of the log*/
struct lhdr_t{

    uint64_t magic_number;
    uint64_t len;
    // locking

};

/* log entry header */
struct lehdr_t{
    char kname[20];
    uint64_t version; // only if a single data item
    uint64_t len; // entry lenth
    HdrType type; // entry type
};

//structure for log traversing
struct walkentry {
    size_t len;
    void *datap;
    uint64_t version;
    uint64_t start_offset;
    char key[KEY_LEN];
    ErrorCode err;

};



namespace  nvs{

    class RootHeap;

// single-shelf heap
    class Log
    {
    public:
        Log() = delete;
        Log(std::string logpath,uint64_t log_size,LogId log_id);
        ~Log();

        ErrorCode append (char *data,size_t size);
        //TODO : get rid of pmem struct from the interface
        ErrorCode appendv(struct iovec *iovp, int iovcnt);
        ErrorCode walk(int (*process_chunk)(const void *buf, size_t len, void *arg),void *arg);
        ErrorCode printLog(); // debug purposes





        void Free (GlobalPtr global_ptr);

    private:

        char* to_addr(uint64_t offset);
        void persist();

        LogId log_id_;
        std::string logPath;
        char *pmemaddr;
        size_t mapped_len;

        size_t size_;
        uint64_t start_offset;
        uint64_t end_offset;
        uint64_t write_offset;
        RootHeap *rootHeap;

    };

}
#endif //NVS_LOG_H

#include <nvs/log.h>
#include <fcntl.h>
#include "log.h"
#include "libpmem.h"

namespace nvs{


    char* Log::to_addr(uint64_t offset) {



    }


    void Log::persist(){

        uint64_t commit_flag = COMMIT_FLAG;
        pmem_drain();
        //commit flag write
        pmem_memcpy_nodrain(&pmemaddr[write_offset],&commit_flag ,WORD_LENGTH);
        pmem_drain();
        write_offset+WORD_LENGTH

    }


    Log::Log(std::string logPath,uint64_t log_size, PoolId pool_id)
            :logPath(logPath),pool_id_(pool_id)
    {


        int is_pmem;
        // if not exist we create the pmem segment
        if ((this->pmemaddr = (char *)pmem_map_file(logPath.c_str(), log_size,
                                      PMEM_FILE_CREATE|PMEM_FILE_EXCL,
                                      0666, &(this->mapped_len), &is_pmem)) == NULL) {
            LOG(fatal) << ("pmem_map_file");
            exit(1);
        }
        if(is_pmem != 1){
            LOG(error) << "not recognized as pmem region";
        }
        //populate the soft state.

        next_append_pos = 1; // for now


    }

    Log::~Log()
    {
        pmem_unmap(this->pmemaddr, this->mapped_len);
        LOG(debug) << "mapped file closed";

        // populate the soft state of the log

    }



    ErrorCode Log::append(char *data, size_t size) {


        ErrorCode errorCode = NO_ERROR;

        LOG(trace) << "data " + std::to_string((long)data) +
                    "size " + std::to_string(size);

        //get the lock

        if(write_offset >= end_offset){
            LOG(fatal) << "no space in log";
            errorCode =  NOT_ENOUGH_SPACE;
            goto end;
        }
        int count;
        if(size > (end_offset-write_offset)){
            LOG(fatal) << "not enough space on log";
            errorCode = NOT_ENOUGH_SPACE;
            goto end;
        }

        pmem_memcpy_nodrain(&pmemaddr[write_offset],data ,size)

        persist(); // commit flag

        end:
            //unlock
            return errorCode;

    }

    ErrorCode Log::appendv(struct iovec *iovp, int iovcnt) {


    }

    ErrorCode Log::metaWalk() {

    }

    ErrorCode Log::walk(int (*process_chunk)(const void *buf, size_t len, void *arg), void *arg) {

        int ret= 0;
        pmemlog_walk(this->lp, 0, process_chunk, arg);

        if(ret < 0){
            return PMEM_ERROR;
        }
        return NO_ERROR;

    }

}


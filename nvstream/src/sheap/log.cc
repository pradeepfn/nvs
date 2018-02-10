#include <nvs/log.h>
#include <fcntl.h>
#include "log.h"
#include "libpmem.h"

namespace nvs{


    char* Log::to_addr(uint64_t offset) {
        return pmemaddr + offset;
    }


    void Log::persist(){

        uint64_t commit_flag = COMMIT_FLAG;
        pmem_drain();
        //commit flag write
        pmem_memcpy_nodrain(&pmemaddr[write_offset],&commit_flag ,WORD_LENGTH);
        pmem_drain();
        write_offset += WORD_LENGTH;
    }


    Log::Log(std::string logPath,uint64_t log_size, LogId log_id)
            :logPath(logPath),log_id_(log_id)
    {

        int is_pmem;

        if ((this->pmemaddr = (char *)pmem_map_file(logPath.c_str(), log_size,
                                      PMEM_FILE_CREATE|PMEM_FILE_EXCL,
                                      0666, &(this->mapped_len), &is_pmem)) != NULL) {
            //new map segment, populate the header
            struct lhdr_t hdr;
            hdr.magic_number = MAGIC_NUMBER;
            hdr.len = this->mapped_len;

            this->start_offset = sizeof(struct lhdr_t);
            this->end_offset = this->mapped_len;
            this->write_offset = this->start_offset;

            pmem_memcpy_persist(this->pmemaddr,&hdr,sizeof(struct lhdr_t));

        }else{
            LOG(warn) << ("pmem_file may be already exist?");
        }


        if((this->pmemaddr == NULL) &&
                ((this->pmemaddr = (char *)pmem_map_file(logPath.c_str(), log_size, 0, 0666,
                        &(this->mapped_len), &is_pmem)) != NULL)){
            //verify the segment header

            struct lhdr_t *hdr = (struct lhdr_t *) this->pmemaddr;
            if(hdr->magic_number != MAGIC_NUMBER){
                LOG(error) << "wrong magic number";
                exit(1);
            }
            this->start_offset = sizeof(struct lhdr_t);
            this->end_offset = this->mapped_len;
            assert(hdr->len == this->mapped_len);
            this->write_offset = -1; //TODO

        }else{
            LOG(error) << "map segment";
            exit(1);
        }


        if(is_pmem != 1){
            LOG(error) << "not recognized as pmem region";
        }

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
        if(size > (end_offset-write_offset)){
            LOG(fatal) << "not enough space on log";
            errorCode = NOT_ENOUGH_SPACE;
            goto end;
        }
        pmem_memcpy_nodrain(&pmemaddr[write_offset],data ,size);
        persist(); // commit flag

        end:
            //unlock
            return errorCode;

    }

    ErrorCode Log::appendv(struct iovec *iovp, int iovcnt) {
        ErrorCode errorCode = NO_ERROR;

        uint64_t tot_cnt = 0;

        //lock
        if(write_offset >= end_offset){
            LOG(fatal) << "no space in log";
            errorCode =  NOT_ENOUGH_SPACE;
            goto end;
        }

        // feasibility check
        for(int i = 0 ; i < iovcnt; i++){
            tot_cnt += iovp[i].iov_len;
        }
        // commit flag
        tot_cnt += WORD_LENGTH;

        if(tot_cnt > (end_offset-write_offset)){
            LOG(fatal) << "not enough space on log";
            errorCode = NOT_ENOUGH_SPACE;
            goto end;
        }

        for(int i = 0 ; i < iovcnt; i++) {
            pmem_memcpy_nodrain(&pmemaddr[write_offset], iovp[i].iov_base, iovp[i].iov_len);
            write_offset +=iovp[i].iov_len;
        }

        persist();

        end:
            //unlock
            return errorCode;

    }


    ErrorCode Log::walk( int (*process_chunk)(const void *buf, size_t len, void *arg), void *arg) {

        //lock
        uint64_t data_offset = start_offset;

        while(data_offset < write_offset){

            if(!(*process_chunk)(&pmemaddr[data_offset], ((struct lehdr_t *) (pmemaddr+data_offset))->len, arg)){
                break;
            }
            data_offset += ((struct lehdr_t *) (pmemaddr+data_offset))->len + WORD_LENGTH;

        }
        //unlock
        return NO_ERROR;

    }

}


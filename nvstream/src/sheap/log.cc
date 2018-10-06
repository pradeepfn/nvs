#include <nvs/log.h>
#include <fcntl.h>
#include "log.h"
#include "common/util.h"


#define NVS_VSHM "nvs_boost_shm"
#define PLOG_MTX "plog_mtx"

namespace nvs{


    char* Log::to_addr(uint64_t offset) {
        return pmemaddr + offset;
    }


    void Log::persist(){

        /* uint64_t commit_flag = COMMIT_FLAG;
        pmem_drain();
        //commit flag write
        pmem_memcpy_nodrain(&pmemaddr[write_offset],&commit_flag ,WORD_LENGTH);

        //We have to fix this properly. Remove commit flag and persist head and tail values
        pmem_memcpy_nodrain(&pmemaddr[write_offset],&commit_flag ,WORD_LENGTH);

        pmem_drain();
        write_offset += WORD_LENGTH;
        struct lhdr_t *hdr = (struct lhdr_t *) this->pmemaddr;
        pmem_memcpy_nodrain((void*)&hdr->tail,&write_offset ,sizeof(uint64_t));
        pmem_drain();
        LOG(debug) << "writeoffset/tail : " << std::to_string(hdr->tail); */
    }


    Log::Log(std::string logPath, uint64_t log_size, LogId log_id, bool is_new) :
		logPath(logPath), log_id(log_id),log_size(log_size),managed_shm(boost::interprocess::open_or_create, NVS_VSHM, 1024)
    {
        int ret;
        std::string plog_mtx = PLOG_MTX + std::to_string(log_id);
        this->mtx = managed_shm.find_or_construct<boost::interprocess::interprocess_mutex>(
                plog_mtx.c_str())();
	    LOG(debug) << "log constructor : logPath , log_size, log_id" + this->logPath + " , " +
					 std::to_string(log_size) + " , " + std::to_string(log_id);

        if(is_new){
        int fd = open(this->logPath.c_str(), O_CREAT |O_RDWR, 0600);
        if (fd <0) {
            std::cout << "root file create failed" << strerror(errno);
            exit(1);
        }
        ret = fallocate(fd,0,0,log_size);
        if (ret < 0) {
            std::cout << "fallocate failed";
            exit(1);
        }
        this->pmemaddr = (char *) mmap(NULL,log_size,PROT_READ| PROT_WRITE, MAP_SHARED,fd,0);

        this->mapped_len = log_size;
		LOG(debug) << "new log created : logPath , log_size, mapped_len   " + this->logPath + " , " +
				 std::to_string(log_size) + " , " + std::to_string(this->mapped_len);
		//new map segment, populate the header
		struct lhdr_t hdr;
		hdr.magic_number = MAGIC_NUMBER;
		hdr.len = this->mapped_len;
		

		this->start_offset = sizeof(struct lhdr_t);
		this->end_offset = this->mapped_len - 1;
		this->write_offset = this->start_offset;
		
		hdr.tail = this->write_offset;
        hdr.head = sizeof(struct lhdr_t);
		(this->pmemaddr, &hdr, sizeof(struct lhdr_t));

		uint64_t k = this->start_offset;
		while (k < this->end_offset) {
			this->pmemaddr[k] = 0;
			k += 4096;
		}

	} else{
            int fd = open(this->logPath.c_str(), O_RDWR, 0600);
            if (fd <0) {
                std::cout << "root file open failed" << strerror(errno);
                exit(1);
            }
            this->pmemaddr = (char *) mmap(NULL,log_size,PROT_READ| PROT_WRITE, MAP_SHARED,fd,0);
        this->mapped_len = log_size;
		LOG(debug) << "using existing log : logPath , log_size, mapped_len   " + this->logPath + " , " +
				std::to_string(log_size) + " , " + std::to_string(this->mapped_len);
		//verify the segment header
		struct lhdr_t *hdr = (struct lhdr_t *) this->pmemaddr;
		if (hdr->magic_number != MAGIC_NUMBER) {
			LOG(error) << "wrong magic number";
			exit(1);
		}

		this->end_offset = this->mapped_len - 1;
		assert(hdr->len == this->mapped_len);

        this->start_offset = hdr->head;
		this->write_offset = hdr->tail;
        LOG(debug)<< "head : " << std::to_string(this->start_offset) << "tail : "
                  << std::to_string(this->write_offset)<< "of persistent log";


	}

}

    Log::~Log()
    {
        int ret = munmap(this->pmemaddr, this->mapped_len);
        if(!ret) {
            LOG(debug) << "mapped file closed";
        }

    }



    ErrorCode Log::append(char *data, size_t size) {

        ErrorCode errorCode = NO_ERROR;
        LOG(trace) << "data " + std::to_string((long)data) +
                    "size " + std::to_string(size);
        //get the lock
        if(write_offset >= end_offset){
            LOG(fatal) << "no space in log";
            errorCode =  NOT_ENOUGH_SPACE;
            exit(1); // we dont want this error for now
            //goto end;
        }
        if(size > (end_offset-write_offset)){
            LOG(fatal) << "not enough space on log";
            errorCode = NOT_ENOUGH_SPACE;
            exit(1); // we dont want this error for now
            //goto end;
        }
        sse_memcpy(&pmemaddr[write_offset],data ,size);
        persist(); // commit flag

        end:
            //unlock
            return errorCode;

    }

    ErrorCode Log::appendv(struct iovec *iovp, int iovcnt) {
        ErrorCode errorCode = NO_ERROR;

        uint64_t tot_cnt = 0;

        //lock this log file
        if(write_offset >= end_offset){
            LOG(fatal) << "no space in log" << "write_offset : " << write_offset <<
                        "end_offset :" << end_offset;
            errorCode =  NOT_ENOUGH_SPACE;
            exit(1); // we dont want this error for now
            //goto end;
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
            exit(1); // we dont want this error for now
            //goto end;
        }
#ifndef _MEMCPY
        for(int i = 0 ; i < iovcnt; i++) {
            sse_memcpy(&pmemaddr[write_offset], iovp[i].iov_base, iovp[i].iov_len);
            write_offset +=iovp[i].iov_len;
        }
        //TODO:
        persist();
#else
        for(int i = 0 ; i < iovcnt; i++) {
            std::memcpy(&pmemaddr[write_offset], iovp[i].iov_base, iovp[i].iov_len);
            write_offset +=iovp[i].iov_len;
        }
        asm_mfence();
#endif
        end:
            //unlock
            return errorCode;

    }

    /*appending multiple variables at once. Each varibale may well be represented by a iovector */
    ErrorCode Log::appendmv(struct iovec **iovpp, int *iovcnt, int iovpcnt) {
            ErrorCode errorCode = NO_ERROR;

            uint64_t tot_cnt = 0;

            //TODO lock this log file
            if(write_offset >= end_offset){
                LOG(fatal) << "no space in log" << "write_offset : " << write_offset <<
                            "end_offset :" << end_offset;
                errorCode =  NOT_ENOUGH_SPACE;
                exit(1); // we dont want this error for now
                //goto end;
            }

            // feasibility check
            for(int i=0; i < iovpcnt; i++){
				for(int j = 0 ; j < iovcnt[i]; j++){
					tot_cnt += iovpp[i][j].iov_len;
				}
            }

            // commit flag
            tot_cnt += WORD_LENGTH;

            if(tot_cnt > (end_offset-write_offset)){
                LOG(fatal) << "not enough space on log";
                errorCode = NOT_ENOUGH_SPACE;
                exit(1); // we dont want this error for now
                //goto end;
            }

    #ifndef _MEMCPY
            for(int i=0; i < iovpcnt ; i++){
				for(int j = 0 ; j < iovcnt[i]; j++) {
					sse_memcpy(&pmemaddr[write_offset], iovpp[i][j].iov_base, iovpp[i][j].iov_len);
					write_offset +=iovpp[i][j].iov_len;
				}
            }
            persist();
    #else
            for(int i=0; i < iovpcnt ; i++){
				for(int j = 0 ; j < iovcnt[i]; j++) {
					std::memcpy(&pmemaddr[write_offset], iovpp[i][j].iov_base, iovpp[i][j].iov_len);
					write_offset +=iovpp[i][j].iov_len;
				}
            }
            asm_mfence();
    #endif
            end:
                //unlock
                return errorCode;

        }


    ErrorCode Log::walk( int (*process_chunk)(const void *buf, size_t len, void *arg), void *arg) {

        //we don't need any locking. head/tail are 64bit values
		struct lhdr_t *hdr = (struct lhdr_t *) this->pmemaddr;
        uint64_t tail = hdr->tail;
        uint64_t head = hdr->head;

		LOG(debug) << "loaded head : " << std::to_string(head) <<" tail : "
                   << std::to_string(tail) << "from persistent log";

        uint64_t tmp_offset = head;
		if(tmp_offset >= tail){
			return ID_NOT_FOUND;
		}

        while(tmp_offset < tail){

            if(!(*process_chunk)(&pmemaddr[tmp_offset],
                                 ((struct lehdr_t *) (pmemaddr+tmp_offset))->len, arg)){
                break;
            }

            //next log header
            tmp_offset += ( sizeof(struct lehdr_t) + // current header length
                            ((struct lehdr_t *) (pmemaddr+tmp_offset))->len + // data lengh
                             WORD_LENGTH);  // commit flag length

        }
        return NO_ERROR;

    }

}


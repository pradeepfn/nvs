#include <nvs/log.h>
#include <fcntl.h>
#include "log.h"
#include "libpmem.h"


#define NVS_VSHM "nvs_boost_shm"
#define PLOG_MTX "plog_mtx"

namespace nvs{


    char* Log::to_addr(uint64_t offset) {
        return pmemaddr + offset;
    }


    void Log::persist(){

        uint64_t commit_flag = COMMIT_FLAG;
        pmem_drain();
        //commit flag write
        pmem_memcpy_nodrain(&pmemaddr[write_offset],&commit_flag ,WORD_LENGTH);

        //We have to fix this properly. Remove commit flag and persist head and tail values
        pmem_memcpy_nodrain(&pmemaddr[write_offset],&commit_flag ,WORD_LENGTH);

        pmem_drain();
        write_offset += WORD_LENGTH;
        struct lhdr_t *hdr = (struct lhdr_t *) this->pmemaddr;
        //TODO: convert to 64 bit atomic write + clflush
        pmem_memcpy_nodrain((void*)&hdr->tail,&write_offset ,sizeof(uint64_t));
        pmem_drain();
        LOG(debug) << "writeoffset/tail : " << std::to_string(hdr->tail);
    }


    Log::Log(std::string logPath, uint64_t log_size, LogId log_id) :
		logPath(logPath), log_id(log_id),log_size(log_size),managed_shm(boost::interprocess::open_or_create, NVS_VSHM, 1024)
    {
        std::string plog_mtx = PLOG_MTX + std::to_string(log_id);
        this->mtx = managed_shm.find_or_construct<boost::interprocess::interprocess_mutex>(
                plog_mtx.c_str())();
	int is_pmem;
	LOG(debug) << "log constructor : logPath , log_size, log_id" + this->logPath + " , " +
					 std::to_string(log_size) + " , " + std::to_string(log_id);
	if ((this->pmemaddr = (char *) pmem_map_file(this->logPath.c_str(),
			log_size,
			PMEM_FILE_CREATE | PMEM_FILE_EXCL, 0666, &(this->mapped_len),
			&is_pmem)) != NULL) {
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
		pmem_memcpy_persist(this->pmemaddr, &hdr, sizeof(struct lhdr_t));

		uint64_t k = this->start_offset;
		while (k < this->end_offset) {
			this->pmemaddr[k] = 0;
			k += 4096;
		}

	} else if ((this->pmemaddr == NULL)
			&& ((this->pmemaddr = (char *) pmem_map_file(this->logPath.c_str(),
					log_size, 0, 0666, &(this->mapped_len), &is_pmem)) != NULL)) {
		LOG(debug) << "using existing log : logPath , log_size, mapped_len   " + this->logPath + " , " +
				std::to_string(log_size) + " , " + std::to_string(this->mapped_len);
		//verify the segment header
		struct lhdr_t *hdr = (struct lhdr_t *) this->pmemaddr;
		if (hdr->magic_number != MAGIC_NUMBER) {
			LOG(error) << "wrong magic number";
			exit(1);
		}

		this->log_end = this->mapped_len - 1;
		assert(hdr->len == this->mapped_len);

        this->start_offset = hdr->head;
		this->write_offset = hdr->tail;
        LOG(debug)<< "head : " << std::to_string(this->start_offset) << "tail : "
                  << std::to_string(this->write_offset)<< "of persistent log";

		// TODO: use huge pages

	} else {
		LOG(error) << "map segment : logPath, log_id " + logPath + " , " + std::to_string(log_id) ;
		exit(1);
	}

	if (is_pmem != 1) {
		LOG(error) << "not recognized as pmem region";
	}

}

    Log::~Log()
    {
        pmem_unmap(this->pmemaddr, this->mapped_len);
        LOG(debug) << "mapped file closed";

        // populate the soft state of the log

    }

    uint64_t append_threshold(uint64_t head, uint64_t tail){

    }

    /* handles log compaction */
    void Log::compact(uint64_t append_len, uint64_t head, uint64_t tail){
        if(append_len > critical_len){ // can't proceed, reclaim log
            this->mtx->unlock();
            this->log_compactor->submit(this->compactor());
        }else if(append_len > append_threshold(head,tail)){
            //TODO: check if we already submitted a log compaction request
            this->log_compactor->submit(this->compactor());
        }
    }



    int Log::next_append(uint64_t apnd_size,uint64_t *apnd_offset){
        // acquire lock before calling this method
        uint64_t vhead = this->pmem_hdr->head;
        uint64_t vtail = this->pmem_hdr->tail;
        uint64_t vwrap_end = this->pmem_hdr->wrap_end;

        if(vtail > vhead) {
            if (apnd_size <= (log_end - vtail)){ //fast path
                *apnd_offset = vtail + 1;
                return NO_ERROR;
            }else if(vhead >= apnd_size  && apnd_size <= vhead){ // wrap around

            }else
                goto truncate;

        }else if(vtail < vhead){
            if(apnd_size < (vhead-vtail)) { //fast path
                *apnd_offset = vtail + 1;
                return NO_ERROR;
            }else
                goto truncate;
        }else{
            LOG(debug) << "wrong execution path";
            exit(1);
        }

        truncate:
            LOG(debug) << "no space in log, truncate!";
            return NOT_ENOUGH_SPACE;
    }

    ErrorCode Log::appendv(struct iovec *iovp, int iovcnt) {
        ErrorCode errorCode = NO_ERROR;

        uint64_t tot_cnt = 0;
        for(int i = 0 ; i < iovcnt; i++){// feasibility check
            tot_cnt += iovp[i].iov_len;
        }
        tot_cnt += WORD_LENGTH;// commit flag

        this->mtx->lock();

        uint64_t apnd_offset;
        int retries =0;
        while(next_append(tot_cnt, &apnd_offset) == NOT_ENOUGH_SPACE){
            if(retries > 1){
                LOG(error) << "Error while sync-truncating log";
            }
            this->compactor();
        }

#ifndef _MEMCPY
        for(int i = 0 ; i < iovcnt; i++) {
            pmem_memcpy_nodrain(&pmemaddr[apnd_offset], iovp[i].iov_base, iovp[i].iov_len);
            write_offset +=iovp[i].iov_len;
        }
        persist();
#else
        for(int i = 0 ; i < iovcnt; i++) {
            std::memcpy(&pmemaddr[write_offset], iovp[i].iov_base, iovp[i].iov_len);
            write_offset +=iovp[i].iov_len;
        }
        asm_mfence();
#endif
        end:
            this->mtx->unlock();
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
					pmem_memcpy_nodrain(&pmemaddr[write_offset], iovpp[i][j].iov_base, iovpp[i][j].iov_len);
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

            if(!(*process_chunk)(&pmemaddr[tmp_offset], ((struct lehdr_t *) (pmemaddr+tmp_offset))->len, arg)){
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


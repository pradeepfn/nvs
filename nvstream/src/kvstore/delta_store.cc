//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <cstdlib>
#include "nvs/log.h"
#include "delta_store.h"


#define LOG_SIZE 700 * 1024 * 1024LLU

namespace nvs{

    DeltaStore::DeltaStore(std::string storeId):
            storeId(storeId)
    {
        ErrorCode ret;
        LogId logId;

        std::string delimeter = "/";
        std::string heapId =  storeId.substr(0,storeId.find(delimeter));
        std::string logIdStr = storeId.substr(storeId.find(delimeter) + delimeter.length());
        logId = std::stoi(logIdStr);

        MemoryManager *mm = MemoryManager::GetInstance();
        ret = mm->FindLog(logId, &(this->log));
        if(ret == ID_NOT_FOUND){
            ret = mm->CreateLog(logId, LOG_SIZE);
            if(ret != NO_ERROR){
                LOG(fatal) << "DeltaStore: error creating log";
                exit(1);
            }
            ret = mm->FindLog(logId, &(this->log)); //TODO: this code block is useless.. remove it
            if(ret != NO_ERROR){
                LOG(fatal) << "DeltaStore: error finding log";
                exit(1);
            }
        }
    }

    DeltaStore::~DeltaStore() {
        LOG(debug) << "destructor, not implemented";


    }

    ErrorCode DeltaStore::create_obj(std::string key, uint64_t size, void **obj_addr)
    {

        int ret;
        void *tmp_ptr;

        LOG(debug) << "Object created : " + key;
        //page aligned object allocation
        uint64_t aligned_size = (size + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
        ret = posix_memalign(&tmp_ptr, PAGE_SIZE, aligned_size);
        if(ret != 0){
            LOG(error) << "error allocating aligned memory";
        }


        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
            Object *obj = new Object(key,size,0,tmp_ptr);
            obj->setVersion(0);
            objectMap[key] = obj;
        }else{
            LOG(fatal) << "not implemented yet";
            exit(1);
        }
        *obj_addr = (uint64_t *)tmp_ptr;

        return NO_ERROR;
    }


    /*
     * this is similar to a commit operation. we do not need the object
     * address and size for put operation as the kvstore already know that
     * detail.
     */
    ErrorCode DeltaStore::put(std::string key, uint64_t version)
    {
        struct iovec *iovp, *next_iovp;
        ErrorCode ret;
        struct lehdr_t l_entry;// stack variable
        uint64_t delta_len = 0;

        std::map<std::string, Object *>::iterator it;
        // first traverse the map and find the key object
        if((it = objectMap.find(key)) != objectMap.end()){
            Object *obj = it->second;
            /* log layout
             *
             * |struct lehdr_t|delta chunks|commit_flag|
             *
             */
            std::vector<struct delta_t> dc = obj->get_delta_chunks();

            //construct IO vector
            iovp = (struct iovec *)malloc(sizeof(struct iovec) * (1+dc.size()));

            std::vector<struct delta_t>::iterator it;
            for(int i = 1 ,it = dc.begin(); it != dc.end; it++,i++){

                iovp[i].iov_base = obj->getPtr() + dc[i].start_offset;
                iovp[i].iov_len = dc[i].len;

                l_entry.deltas[i-1].start_offset = dc[i].start_offset ;
                l_entry.deltas[i-1].len = dc[i].len;
                delta_len += it->len;
            }

            //populate log header
            l_entry.version = version;
            snprintf(l_entry.kname, KEY_LEN,"%s", obj->getName().c_str());
            l_entry.len = obj->getSize();
            l_entry.delta_len = delta_len;
            l_entry.type = HdrType::single;

            iovp[0].iov_base = &l_entry;
            iovp[0].iov_len = sizeof(l_entry);


            ret = this->log->appendv(iovp,iovcnt);
            if(ret != NO_ERROR){
                LOG(fatal) << "Store: append failed";
                exit(1);
            }
            free(iovp);
            /* 1. increase the soft state
             * 2. write protect the volatile buffers
             * 3. reset the bitset vector
             */
            obj->setVersion(version);
            enable_write_protection(obj->getPtr(), obj->get_aligned_size());
            obj->reset_bit_vector();
            return NO_ERROR;
        }

        LOG(error) << "element not found";
        return ELEM_NOT_FOUND;
    }


    /*
     * implements the snapshot method.
     *
     */
    ErrorCode DeltaStore::put_all() {

        struct iovec *iovp, *tmp_iovp;
        ErrorCode ret = NO_ERROR;
        struct lehdr_t *l_entry, *tmp_l_entry;
        uint64_t nvar, iovcnt,tot_cnt=0;

        if(!(nvar = objectMap.size())){
            LOG(warning) << "no objects found";
        }
        /* figure out number of iovec needed.
            1 - lehdr of type multiple
            2* nvars - each variable has lehdr of type 'single' and data
        */

        iovcnt = 1+2*nvar;
        iovp = (struct iovec *)malloc(sizeof(struct iovec) * iovcnt);
        l_entry = (struct lehdr_t *)malloc(sizeof(struct lehdr_t) * 1+nvar);
        /* start with inner elements */
        tmp_iovp = iovp+1;
        tmp_l_entry = l_entry+1;
        std::map<std::string, Object *>::iterator it;
        for(it = objectMap.begin(); it != objectMap.end(); it++){
            Object *obj = it->second;

            //populate header
            tmp_l_entry->version = obj->getVersion()+1;
            snprintf(tmp_l_entry->kname, KEY_LEN,"%s", obj->getName().c_str());
            tmp_l_entry->len = obj->getSize();
            tmp_l_entry->type = single;
            /* populate copy vector */
            tmp_iovp->iov_base = tmp_l_entry;
            tmp_iovp->iov_len = sizeof(struct lehdr_t);
            tmp_iovp++;

            tmp_iovp->iov_base = obj->getPtr();
            tmp_iovp->iov_len = obj->getSize();
            tot_cnt += (sizeof(struct lehdr_t) + obj->getSize());

            tmp_iovp++;
            tmp_l_entry++;
        }

        l_entry->version = 0;
        l_entry->type = multiple;
        l_entry->len  = tot_cnt;

        iovp->iov_base = l_entry;
        iovp->iov_len = sizeof(struct lehdr_t);

        if(this->log->appendv(iovp,iovcnt) != NO_ERROR){
            LOG(error) << "append error";
            ret = PMEM_ERROR;
            goto end;
        }

        //increase the soft state
        for(it = objectMap.begin(); it != objectMap.end(); it++) {
            Object *obj = it->second;
            obj->setVersion(obj->getVersion()+1);
        }

        end:
        free(l_entry);
        free(iovp);
        return ret;
    }



    static int
    process_chunks(const void *buf, size_t len, void *arg)
    {

        // getting result from the callback -- walkentry structure
        struct walkentry *wentry = (struct walkentry *) arg;
        wentry->err = ID_NOT_FOUND;

        struct lehdr_t *hdr = (struct lehdr_t *) buf;

        if(hdr->type == HdrType::single){
            char *data = (char *)buf + sizeof(struct lehdr_t);
            if(!strncmp(hdr->kname, wentry->key,KEY_LEN) && hdr->version == wentry->version){
                wentry->datap = data;
                wentry->len = hdr->len;
                /* copy the delta chunk metadata */
                memcpy(&wentry->deltas, &hdr->deltas, sizeof(struct ledelta_t) * 5);
                wentry->delta_len = hdr->delta_len;
                wentry->err = NO_ERROR;

                return 0;
            }else{
                LOG(debug) << "looking for : " + std::string(wentry->key) + " , " +  std::to_string(wentry->version) +
                              "   current entry : " + std::string(hdr->kname) + " , " + std::to_string(hdr->version);
                return 1; // process next chunk
            }

        }else if(hdr->type == HdrType::multiple){
            LOG(error) << "not suported";
        }else if(hdr->type == HdrType::delta){
            LOG(error) << "not supported";
        }else{
            LOG(error) << "wrong chunk header type";
            exit(1);
        }
    }

    /*
     *  the object address is redundant here
     */
    ErrorCode DeltaStore::get(std::string key, uint64_t version, void *obj_addr)
    {
        LOG(error) << "not supported";

    }

    ErrorCode DeltaStore::get_with_malloc(std::string key, uint64_t version, void **addr) {

        struct walkentry wentry;
        wentry.version = version;
        wentry.start_offset = 0;
        snprintf(wentry.key,KEY_LEN,"%s",key.c_str());

        this->log->walk(process_chunks, &wentry);

        if(wentry.err != NO_ERROR){
            LOG(debug) << "Object not found : " + key + " , " + std::to_string(version);
            return ID_NOT_FOUND;
        }

        //find the object from the object map, if not found -- > create one
        // this is a consumer side volatile state
        std::map<std::string, Object *>::iterator it;
        Object *obj;
        if((it=objectMap.find(key)) != objectMap.end()){
            obj = it->second;
            /* copying deltas */
            delta_memcpy(obj->getPtr(),wentry.datap ,
                         &wentry.deltas[0],wentry.delta_len);
            obj->setVersion(version);
            *addr = obj->getPtr();
            return NO_ERROR;

        }else{
            void *tmp_ptr = malloc(wentry.len);
            obj = new Object(key,wentry.len,version,tmp_ptr);
            objectMap[key] = obj;
            /* copying delta is not enough, we have construct the full data*/

            return NO_ERROR;
        }

    }

    Key * DeltaStore::findKey(std::string key) {

    }


    void DeltaStore::stats() {

    }



    void DeltaStore::delta_copy(char *dst, char *src,
                                struct ledelta_t *deltas, uint64_t delta_len){
        uint64_t src_offset = 0;
        for(int i = 0; i < delta_len; i ++){
            memcpy(dst+deltas[i].start_offset,src + src_offset, deltas[i].len);
            src_offset += deltas[i].len;
        }
    }
}



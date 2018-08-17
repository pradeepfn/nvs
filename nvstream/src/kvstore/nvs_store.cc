//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <cstdlib>
#include "nvs/log.h"
#include "nvs_store.h"


namespace nvs{

    NVSStore::NVSStore(std::string storeId):
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
                LOG(fatal) << "NVSStore: error creating log";
                exit(1);
            }
            ret = mm->FindLog(logId, &(this->log)); //TODO: this code block is useless.. remove it
            if(ret != NO_ERROR){
                LOG(fatal) << "NVSStore: error finding log";
                exit(1);
            }
        }
    }

    NVSStore::~NVSStore() {
        LOG(debug) << "destructor, not implemented";


    }

    ErrorCode NVSStore::create_obj(std::string key, uint64_t size, void **obj_addr)
    {

        LOG(debug) << "Object created : " + key;
        void *tmp_ptr = malloc(size);
        Object *obj = new Object(key,size,0,tmp_ptr);
        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
            obj->setVersion(0);
            objectMap[key] = obj;
            addrMap[(uint64_t) tmp_ptr] = key;
        }else{
        	LOG(fatal) << "object key already exists : " + key;
            exit(1);
        }
        *obj_addr = (uint64_t *)tmp_ptr;

        return NO_ERROR;
    }




    ErrorCode NVSStore::free_obj(void *obj_addr){
    		std::map<uint64_t, std::string>::iterator it1;
    		std::map<std::string, Object *>::iterator it2;
    		it1 = addrMap.find((uint64_t)obj_addr);
    		if(it1 == addrMap.end()){
    			LOG(fatal) << "no object found in the nvs address map";
    			exit(1);
    		}
    		it2= objectMap.find(it1->second);
    		if(it2 == objectMap.end()){
    			LOG(fatal) << "no object found named : " + it1->second;
    			exit(1);
    		}

    		objectMap.erase(it2);//remove from the objectMap
    		addrMap.erase(it1); //remove from the addrMap
    		delete it2->second; // delete object
    		free(obj_addr); //free the address

    		return NO_ERROR;
    }


    /*
     * this is similar to a commit operation. we do not need the object
     * address and size for put operation as the kvstore already know that
     * detail.
     */
    uint64_t NVSStore::put(std::string key, uint64_t version)
    {
			LOG(debug) << "Object put : " + key ;
            struct iovec *iovp, *next_iovp;
            uint64_t total_size=0;
            struct lehdr_t l_entry;// stack variable

            std::map<std::string, Object *>::iterator it;
            // first traverse the map and find the key object
            if((it = objectMap.find(key)) != objectMap.end()){
                Object *obj = it->second;
                //uint64_t version = obj->getVersion();

                //construct IO vector
                int iovcnt = 2; // header + data
                iovp = (struct iovec *)malloc(sizeof(struct iovec) * iovcnt);


                //populate header
                l_entry.version = version;
                snprintf(l_entry.kname, KEY_LEN,"%s", obj->getName().c_str());
                l_entry.len = obj->getSize();
                l_entry.type = HdrType::single;

                iovp[0].iov_base = &l_entry;
                iovp[0].iov_len = sizeof(l_entry);

                //data
                iovp[1].iov_base = obj->getPtr();
                iovp[1].iov_len = obj->getSize();
                total_size = obj->getSize() + sizeof(l_entry);

				fflush(stdout);
				fflush(stderr);

                if(this->log->appendv(iovp,iovcnt)!= NO_ERROR){
                    LOG(fatal) << "Store: append failed";
                    exit(1);
                }
                free(iovp);
                //increase the soft state
                obj->setVersion(version);
                return NO_ERROR;
            }

            LOG(error) << "element not found";
            return ELEM_NOT_FOUND;
    }


    /*
     * implements the snapshot method.
     *
     */
    uint64_t NVSStore::put_all() {

        struct iovec *iovp, *tmp_iovp;
        uint64_t total_size=0;
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
        l_entry = (struct lehdr_t *)malloc(sizeof(struct lehdr_t) * (1+nvar));
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
        total_size = tot_cnt + sizeof(lehdr_t);

        if(this->log->appendv(iovp,iovcnt) != NO_ERROR){
            LOG(error) << "append error";
            total_size = PMEM_ERROR;
            goto end;
        }

        //increase the soft state
        for(it = objectMap.begin(); it != objectMap.end(); it++) {
            Object *obj = it->second;
            obj->setVersion(obj->getVersion()+1);
        }

        end:
            free((void *)iovp);
            free((void *)l_entry);

            return total_size;
    }


/*
    static int
    processLog(const void *buf, size_t len, void *arg)
    {
        // getting result from the callback -- walkentry structure
        struct walkentry *wentry = (struct walkentry *) arg;

        const void *endp = (char *)buf + len;
        buf = (char *)buf + wentry->start_offset;
        while(buf < endp){
            struct lehdr_t *headerp = (struct lehdr_t *) buf;
            buf = (char *)buf + sizeof(struct logentry);

            //check for key and version
            if(!strncmp(headerp->kname, wentry->key,64) && headerp->version == wentry->version){
                wentry->datap = (void *)buf;
                wentry->len = headerp->len;
                wentry->err = NO_ERROR;
                return 0; // no error
            }else{
                LOG(debug) << "looking for : " + std::string(wentry->key) + " , " +  std::to_string(wentry->version) +
                              "   current entry : " + std::string(headerp->key) + " , " + std::to_string(headerp->version);
            }
            buf = (char *) buf + headerp->len;
        }
        wentry->err = ID_NOT_FOUND;
        return -1;
    }*/



    static int
    process_chunks(const void *buf, size_t len, void *arg)
    {
		LOG(debug) << "starting log chunk processing";
        // getting result from the callback -- walkentry structure
        struct walkentry *wentry = (struct walkentry *) arg;
        wentry->err = ID_NOT_FOUND;

        struct lehdr_t *lehdr = (struct lehdr_t *) buf;

        if(lehdr->type == HdrType::single){
            char *data = (char *)buf + sizeof(struct lehdr_t);
            if(!strncmp(lehdr->kname, wentry->key,KEY_LEN) && lehdr->version == wentry->version){
				LOG(debug) <<"processing chunk, name : " << std::string(lehdr->kname) << "length : " << lehdr->len;
                wentry->datap = data;
                wentry->len = lehdr->len;
                wentry->err = NO_ERROR;
                return 0;
            }else{
                LOG(debug) << "looking for : " + std::string(wentry->key) + " , " +  std::to_string(wentry->version) +
                              "   current entry : " + std::string(lehdr->kname) + " , " + std::to_string(lehdr->version);
                return 1; // process next chunk
            }

        }else if(lehdr->type == HdrType::multiple){
			LOG(debug) << "processing batch chunk";
            /* inner data packet starts after the outer header */
            char *i_buf = (char *)buf + sizeof(struct lehdr_t);

            /* traverse the inner data packet */

            struct lehdr_t *i_hdr = (struct lehdr_t *)i_buf;
            char *i_data = (char *)i_buf + sizeof(struct lehdr_t);
            uint64_t i_index = sizeof(struct lehdr_t) + i_hdr->len;

            while(i_index < lehdr->len){
                if(!strncmp(i_hdr->kname,wentry->key, KEY_LEN) && i_hdr->version == wentry->version){
                    wentry->datap = i_data;
                    wentry->len = i_hdr->len;
                    wentry->err = NO_ERROR;
                    return 0;
                }
                /* process the next data point in inner-chunk */
                i_hdr = (struct lehdr_t *)(i_buf + i_index);
                i_data = i_buf + i_index + sizeof(struct lehdr_t);
                i_index += (sizeof(struct lehdr_t) + i_hdr->len);
            }

            // we did not find the data within our inner chunk. process next chunk
            LOG(debug) << "variable not found in chunk:multiple";
            return 1;

        }else if(lehdr->type == HdrType::delta){
            LOG(error) << "not supported";
        }else{
            LOG(error) << "wrong chunk header type";
            exit(1);
        }
    }

    /*
     *  the object address is redundant here
     */
    ErrorCode NVSStore::get(std::string key, uint64_t version, void *obj_addr)
    {
		LOG(debug) << "fetching object with, name : " << key << "version : "<<std::to_string(version);
        struct walkentry wentry;
        wentry.version = version;
        wentry.start_offset = 0;
        snprintf(wentry.key,KEY_LEN,"%s",key.c_str());

		LOG(debug) << "starting log walk";
        //this->log->walk(process_chunks, &wentry);
		if(this->log->walk(process_chunks, &wentry)!=NO_ERROR){
	        return ID_NOT_FOUND;
		}



        if(wentry.err != NO_ERROR){
            LOG(debug) << "Object not found : " + key + " , " + std::to_string(version);
            return ID_NOT_FOUND;
        }

        //*obj_addr = wentry.datap;
		LOG(debug)<< "copying object of length :" << wentry.len;
        memcpy(obj_addr,wentry.datap,wentry.len);




		LOG(debug)<< "object found, returning..";
        return NO_ERROR;

    }

    ErrorCode NVSStore::get_with_malloc(std::string key, uint64_t version, void **addr) {

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
        }else{
            void *tmp_ptr = malloc(wentry.len);
            obj = new Object(key,wentry.len,version,tmp_ptr);
            objectMap[key] = obj;
        }
        obj->setVersion(version);
        //*obj_addr = wentry.datap;
        memcpy(obj->getPtr(),wentry.datap,wentry.len);
        *addr = obj->getPtr();
        return NO_ERROR;
    }

    Key * NVSStore::findKey(std::string key) {

    }


    void NVSStore::stats() {

    }

}

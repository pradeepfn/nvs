//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include <cstdlib>
#include "nvs/log.h"
#include "delta_store.h"




namespace nvs{

	// global delta-store object that handles the page protection fault

	DeltaStore *ds_object;
	struct sigaction old_sa; // old sigaction structure
	 void delta_handler_wrapper(int sig, siginfo_t *si, void *unused){
		 ds_object->delta_handler(sig,si,unused);
	 }

void DeltaStore::delta_handler(int sig, siginfo_t *si, void *unused) {
	if (si != NULL && si->si_addr != NULL) {
		void *pageptr;
		uint64_t offset = 0;
		pageptr = si->si_addr;

		std::map<std::string, Object *>::iterator it;
		for (it = objectMap.begin(); it != objectMap.end(); it++) {
			offset = (uint64_t) pageptr - (uint64_t) it->second->getPtr();
			if (offset >= 0 && offset <= it->second->get_aligned_size()) { // the address belong to this object
					/* if(isDebugEnabled()){
					 printf("[%d] starting address of the matching chunk %p\n",lib_process_id, s->ptr);
					 }*/
				//get the page start
				pageptr = (void *) ((long) si->si_addr & ~(PAGE_SIZE - 1));
				disable_protection(pageptr, PAGE_SIZE);

				uint64_t page_index = offset >> 12;
				it->second->set_modified_bit(page_index);
				return;
			}
		}
		/* the raised signal not belong to our tracking addresses */
		call_oldhandler(sig, &old_sa);
	}
}


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
        /* install the signal handler for page-modification detection */
        install_sighandler(&delta_handler_wrapper,&old_sa);

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
        	LOG(fatal) << "object key already exists : " + key;
            exit(1);
        }

        *obj_addr = (uint64_t *)tmp_ptr;

        return NO_ERROR;
    }


    ErrorCode DeltaStore::free_obj(void *obj_addr) {
		std::map<uint64_t, std::string>::iterator it1;
		std::map<std::string, Object *>::iterator it2;
		it1 = addrMap.find((uint64_t) obj_addr);
		if (it1 == addrMap.end()) {
			LOG(fatal) << "no object found in the dnvs address map";
			exit(1);
		}
		it2 = objectMap.find(it1->second);
		if (it2 == objectMap.end()) {
			LOG(fatal) << "no object found named : " + it1->second;
			exit(1);
		}

		objectMap.erase(it2); //remove from the objectMap
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
    uint64_t DeltaStore::put(std::string key, uint64_t version)
    {
        struct iovec *iovp, *next_iovp;
        uint64_t total_size=0;
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
            int i;
            for(it = dc.begin(),i=1; it != dc.end(); it++,i++){

                iovp[i].iov_base = (char *)obj->getPtr() + it->start_offset;
                iovp[i].iov_len = it->len;

                l_entry.deltas[i-1].start_offset = it->start_offset ;
                l_entry.deltas[i-1].len = it->len;
                delta_len += it->len;
            }

            //populate log header
            l_entry.version = version;
            snprintf(l_entry.kname, KEY_LEN,"%s", obj->getName().c_str());
            l_entry.len = obj->getSize();
            l_entry.delta_len = delta_len;
            l_entry.type = HdrType::delta;

            iovp[0].iov_base = &l_entry;
            iovp[0].iov_len = sizeof(l_entry);

            total_size = delta_len + sizeof(l_entry);

            if(this->log->appendv(iovp,dc.size()+1) != NO_ERROR){
                LOG(fatal) << "Store: append failed";
                exit(1);
            }
            free(iovp);
            /* 1. increase the soft state
             * 2. write protect the volatile buffers
             * 3. reset the bitset vector
             */
            obj->setVersion(version);
            /* delta compression tracking */
            enable_write_protection(obj->getPtr(), obj->get_aligned_size());

            obj->reset_bit_vector();
            return total_size;
        }

        LOG(error) << "element not found";
        return ELEM_NOT_FOUND;
    }


    /*
     * implements the snapshot method.
     *
     */
    uint64_t DeltaStore::put_all() {

        uint64_t total_size=0;
        uint64_t iovpcnt,tot_cnt=0;

        if(!(objectMap.size())){
            LOG(warning) << "no objects found";
        }


        iovpcnt = objectMap.size() + 1;
        struct iovec **iovpp = (struct iovec **)malloc(sizeof(struct iovec *) * iovpcnt );
        int *iovcntp = (int *) malloc(sizeof(int) * iovpcnt);
        struct lehdr_t *l_entryp = (struct lehdr_t *)malloc(sizeof(struct lehdr_t) * iovpcnt);


        std::map<std::string, Object *>::iterator mapIt;
        int i;
        for(mapIt = objectMap.begin(), i=1; mapIt != objectMap.end(); mapIt++, i++){
            Object *obj = mapIt->second;
            std::vector<struct delta_t> dc = obj->get_delta_chunks();
            uint64_t delta_len = 0;


            //construct 2d IO vector. IMPROVE: merge two arrays in to a struct
		   iovpp[i] = (struct iovec *)malloc(sizeof(struct iovec) * (1+dc.size()));
		   iovcntp[i] = 1+dc.size();

		   std::vector<struct delta_t>::iterator it;
		   int j;
		   for(it = dc.begin(),j=1; it != dc.end(); it++,j++){

			   iovpp[i][j].iov_base = (char *)obj->getPtr() + it->start_offset;
			   iovpp[i][j].iov_len = it->len;

			   l_entryp[i].deltas[j-1].start_offset = it->start_offset ;
			   l_entryp[i].deltas[j-1].len = it->len;
			   delta_len += it->len;
		   }

		   //populate log header
		   l_entryp[i].version = obj->getVersion()+1;;
		   snprintf(l_entryp[i].kname, KEY_LEN,"%s", obj->getName().c_str());
		   l_entryp[i].len = obj->getSize();
		   l_entryp[i].delta_len = delta_len;
		   l_entryp[i].type = HdrType::delta;

		   iovpp[i][0].iov_base = &l_entryp[i];
		   iovpp[i][0].iov_len = sizeof(struct lehdr_t);

		   tot_cnt += ( delta_len + sizeof(struct lehdr_t));
        }

        iovpp[0] = (struct iovec *)malloc(sizeof(struct iovec));
        iovcntp[0] = 1;

        l_entryp[0].version = 0;
        l_entryp[0].type = HdrType::multiple;
        l_entryp[0].len  = tot_cnt;

        iovpp[0][0].iov_base = &l_entryp[0];
        iovpp[0][0].iov_len = sizeof(struct lehdr_t);

        total_size = tot_cnt + sizeof(struct lehdr_t);

        if(this->log->appendmv(iovpp,iovcntp, iovpcnt) != NO_ERROR){
            LOG(error) << "append error";
            total_size = PMEM_ERROR;
            goto end;
        }

        for(mapIt = objectMap.begin(); mapIt != objectMap.end(); mapIt++) {
            Object *obj = mapIt->second;
            obj->setVersion(obj->getVersion()+1);
            enable_write_protection(obj->getPtr(), obj->get_aligned_size());
            obj->reset_bit_vector();
        }

        end:
        //TODO: free the volatile structures
        return total_size;
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
            delta_memcpy((char *)obj->getPtr(), (char *)wentry.datap ,
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



    void DeltaStore::delta_memcpy(char *dst, char *src,
                                struct ledelta_t *deltas, uint64_t delta_len){
        uint64_t src_offset = 0;
        for(int i = 0; i < delta_len; i ++){
            memcpy(dst+deltas[i].start_offset,src + src_offset, deltas[i].len);
            src_offset += deltas[i].len;
        }
    }


}



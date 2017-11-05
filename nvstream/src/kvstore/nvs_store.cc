//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include "nvs/log.h"
#include "nvs_store.h"
#include "logentry.h"

namespace nvs{


    NVSStore::NVSStore(std::string storeId):
            storeId(storeId),pid(pid)
    {

    }

    NVSStore::~NVSStore() {}

    ErrorCode NVSStore::create_obj(std::string key, uint64_t size, void **obj_addr)
    {

        void *tmp_ptr = malloc(size);
        Object *obj = new Object(key,size,0,tmp_ptr);
        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
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
    ErrorCode NVSStore::put(std::string key, uint64_t version)
    {
            struct iovec *iovp, *next_iovp;
            ErrorCode ret;
            struct logentry l_entry;

            std::map<std::string, Object *>::iterator it;
            // first traverse the map and find the key object
            if((it = objectMap.find(key)) != objectMap.end()){
                Object *obj = it->second;
                uint64_t version = obj->getVersion();

                //construct IO vector
                int iovcnt = 2; // data + header
                iovp = (struct iovec *)malloc(sizeof(struct iovec) * iovcnt);

                next_iovp = iovp;
                //populate header
                l_entry.version = version;
                snprintf(l_entry.key, 64,"%s", obj->getName().c_str());
                l_entry.len = obj->getSize();

                next_iovp->iov_base = &l_entry;
                next_iovp->iov_len = sizeof(l_entry);
                next_iovp++;
                //data

                next_iovp->iov_base = obj->getPtr();
                next_iovp->iov_len = obj->getSize();


                ret = this->log->appendv(iovp,iovcnt);
                if(ret != NO_ERROR){
                    LOG(fatal) << "Store: append failed";
                    exit(1);
                }
                free(iovp);
                //increase the soft state
                obj->setVersion(version+1);
                return NO_ERROR;
            }
            LOG(error) << "element not found";
            return ELEM_NOT_FOUND;
    }


    static int
    processLog(const void *buf, size_t len, void *arg)
    {
        struct walkentry *wentry = (struct walkentry *) arg;

        const void *endp = (char *)buf + len;
        buf = (char *)buf + wentry->start_offset;
        while(buf < endp){
            struct logentry *headerp = (struct logentry *) buf;
            buf = (char *)buf + sizeof(struct logentry);

            //check for key and version
            if(headerp->key   && headerp->version == wentry->version){
                wentry->datap = (void *)buf;
                wentry->len = headerp->len;
                return 0; // no error
            }else{
                LOG(fatal) << "not implemented";
                exit(1);
            }
        }
    }



    /*
     *  the object address is redundant here
     */
    ErrorCode NVSStore::get(std::string key, uint64_t version, void **obj_addr)
    {
       /* 1. get hold of the read lock for the key
          2. Lookup key and version - the returned object contains the metadata to
          construct the data
          3. copy data in to return object
          4. given object can be null or already returned object address.
        */
    }


    Key * NVSStore::findKey(std::string key) {

    }

}

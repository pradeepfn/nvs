/*
 * null_store.h
 *
 *  Created on: May 2, 2018
 *      Author: pradeep
 */

#ifndef SRC_KVSTORE_NULL_STORE_H_
#define SRC_KVSTORE_NULL_STORE_H_


#include <nvs/store.h>



namespace nvs{
    class NullStore: public Store{

    public:

        NullStore(Store *st){}

        ~NullStore(){}

        ErrorCode create_obj(std::string key, uint64_t size, void **obj_addr){
        	*obj_addr = malloc(size);
        	return NO_ERROR;
        }


        ErrorCode free_obj(void *obj_addr){
        	free(obj_addr);
        	return NO_ERROR;
        }

        uint64_t put(std::string key, uint64_t version){return 0;}

        uint64_t put_all(){return 0;}

        ErrorCode get(std::string key, uint64_t version, void *obj_addr){return NO_ERROR;}

        ErrorCode get_with_malloc(std::string key, uint64_t version, void **addr){return NO_ERROR;}

        std::string get_store_id(){ return 0;}

        void stats(){return;}

    private:
    };

}





#endif /* SRC_KVSTORE_NULL_STORE_H_ */

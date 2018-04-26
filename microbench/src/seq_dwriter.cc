#include <string>
#include "nvs/store.h"
#include "nvs/store_manager.h"
#include "nvs/log.h"


#define STORE_ID "dseq_store"


#define PAGE_SIZE 4096


uint64_t global_counter = 3;

void init_data(char *data){

	for(int i=0; i < 10*PAGE_SIZE; i++){
		data[i] = 0;
	}

}

void modify(char *data){
	for(int i = global_counter; i < 10; i++){
		if((i%2) == 0){
			data[i * PAGE_SIZE] = global_counter;
		}
	}
	global_counter+=4;
}

void print(char *data){
	return; // not implemented
}

int main(int argc, char *argv[]){

    if(argc < 2) {
        std::cout << "invalid arguments" << std::endl;
        return 1;
    }
    std::string rank = std::string(argv[1]);


    uint64_t  version = 0;
    int **tmpvar;
    nvs::ErrorCode ret;
    std::string store_name = std::string(STORE_ID) + "/" + rank;

    nvs::init_log(nvs::SeverityLevel::all,"");
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);

    uint64_t size = 10 * PAGE_SIZE;

    void *ptr1;
    ret =  st->create_obj("var3", size, &ptr1);
    init_data((char *)ptr1);

    void *ptr2;
    ret =  st->create_obj("var4", size, &ptr2);
    init_data((char *)ptr2);


    st->put("var3", ++version);
    st->put("var4", ++version);

    modify((char *)ptr1);
    modify((char *)ptr2);

    st->put("var3", ++version);
    st->put("var4", ++version);

    st->stats();
    return 0;
}

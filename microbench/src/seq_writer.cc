#include <string>
#include "nvs/store.h"
#include "nvs/store_manager.h"
#include "nvs/log.h"


#define STORE_ID "seq_store"

#define row 3
#define column 4

void init_data(int **var){

    int *const data = (int *)(var + row);

    int i,j;
    for(i=0; i< row; i++){
        var[i] = data + i * column;
    }

    for(i=0; i< 3; i ++){
        for(j=0; j < 4; j++){
            var[i][j] = i*column + (j+1);
            printf("%d ", var[i][j]);
        }
    }
    printf("\n");
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
    int var[row][column] = {{1,2,3,4},{5,6,7,8},{9,10,11,12}};
    uint64_t size = row * sizeof(*tmpvar) + row*(column * sizeof(**tmpvar));

    nvs::init_log(nvs::SeverityLevel::all,"");
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);

    void *ptr1;
    ret =  st->create_obj("var3", size, &ptr1);
    init_data((int **)ptr1);

    void *ptr2;
    ret =  st->create_obj("var4", size, &ptr2);
    init_data((int **)ptr2);


    st->put("var3", ++version);
    st->put("var4", ++version);
    st->put("var3", ++version);
    st->put("var4", ++version);
	

	//free ptr1
	st->free_obj(ptr1);

    st->put_all();


    st->stats();
    return 0;
}

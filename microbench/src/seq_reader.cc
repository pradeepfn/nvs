#include <string>
#include "nvs/store.h"
#include "nvs/store_manager.h"


#define STORE_ID "seq_store"

#define row 3
#define column 4

int main(){

    uint64_t  version = 0;
    int **var3;
    nvs::ErrorCode ret;
    std::string store_name = STORE_ID + std::string("/0");
    uint64_t size = row * sizeof(*var3) + row*(column * sizeof(**var3));
    void *ptr = malloc(size);
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);
    //ret = st->get("var3", 1, ptr);
    ret = st->get_with_malloc("var3", 3, &ptr);
    if(ret != nvs::NO_ERROR){
        printf("get failed.\n");
        exit(1);
    }

    //formatting mem space
    var3 = (int **)ptr;
    int *const data = (int *)(var3 + row);
    int i,j;
    for(i=0; i< row; i++){
        var3[i] = data + i * column;
    }

    for(i=0; i< 3; i ++){
        for(j=0; j < 4; j++){
            printf("%d ", var3[i][j]);
        }
    }
    st->stats();

    return 0;
}
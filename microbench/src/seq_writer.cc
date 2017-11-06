#include <string>
#include "nvs/store.h"
#include "nvs/store_manager.h"
#include "nvs/log.h"


#define STORE_ID "seq_store"

#define row 3
#define column 4

int main(){

    uint64_t  version = 0;
    int **var3;
    nvs::ErrorCode ret;
    int var[row][column] = {{1,2,3,4},{5,6,7,8},{9,10,11,12}};
    uint64_t size = row * sizeof(*var3) + row*(column * sizeof(**var3));
    void *ptr;
    nvs::init_log(nvs::SeverityLevel::all,"");
    nvs::Store *st = nvs::StoreManager::GetInstance(STORE_ID);
    ret =  st->create_obj("var3", size, &ptr);

    var3 = (int **)ptr;

    int *const data = (int *)(var3 + row);

    int i,j;
    for(i=0; i< row; i++){
            var3[i] = data + i * column;
    }

    for(i=0; i< 3; i ++){
        for(j=0; j < 4; j++){
            var3[i][j] = i*column + (j+1);
            printf("%d ", var3[i][j]) ;
        }
    }

    st->put("var3", ++version);

    st->stats();
    return 0;
}
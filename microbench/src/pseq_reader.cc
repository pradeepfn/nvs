#include <iostream>
#include "nvs/store.h"
#include "nvs/store_manager.h"
#include "nvs/log.h"

#define STORE_ID "gtc_store"


/*
 * This consumer reads variables from gtc output. The variable reads are
 * hard-coded at the moment
 */

#define GTC_VARS {"array_1d"}
#define GTC_NVARS 1
//#define GTC_VARS {"phi" , "phisave" , "zelectron" , "zelectron0" ,"zion" , "zion0", "zonale", "zonali"}
//#define GTC_NVARS 8
#define N_VERSIONS 3

int main(int argc, char **argv)
{

    if(argc < 2) {
        std::cout << "invalid arguments" << std::endl;
        return 1;
    }
    std::string rank = std::string(argv[1]);

    std::string var_array[] = GTC_VARS;

    std::cout << "Consumer - starting computation" << std::endl;

    std::string store_name = std::string(STORE_ID) + std::string("/") + rank;
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);
    nvs::init_log(nvs::SeverityLevel::all,"");

    // we have to read variables one by one. Bulk read make no sense...

    void *dataPtr;
    for(int v =1; v < N_VERSIONS ;v++)
        for(int i =0; i < GTC_NVARS; i++){
            st->get_with_malloc(var_array[i],v, &dataPtr);
        }
    st->stats();
    return 0;
}
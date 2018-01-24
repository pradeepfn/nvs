#include <mpi.h>
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
//#define GTC_VARS {"phi" , "zion"}
#define N_VERSIONS 10

int main(int argc, char **argv)
{
    int proc_id;
    char hostname[256];

    std::string var_array[] = GTC_VARS;


    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
    //gethostname(hostname,255);

    //printf("Hello world!  I am process number: %d on host %s\n", rank, hostname);


    if(proc_id == 0)
        std::cout << "Consumer - starting computation" << std::endl;

    std::string store_name = std::string(STORE_ID) + std::string("/") +std::to_string(proc_id);
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);
    nvs::init_log(nvs::SeverityLevel::all,"");

    // we have to read variables one by one. Bulk read make no sense...

    void *dataPtr;
    for(int v =1; v < 3;v++)
    for(int i =0; i < GTC_NVARS; i++){
        st->get_with_malloc(var_array[i],v, &dataPtr);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    st->stats();
    MPI_Finalize();

    return 0;
}
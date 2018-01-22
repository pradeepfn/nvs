#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#include "nvs/store.h"
#include "nvs/store_manager.h"

#define STORE_ID "gtc_store"

int main(int argc, char **argv)
{
    int mype;
    char hostname[256];

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mype);
    //gethostname(hostname,255);

    //printf("Hello world!  I am process number: %d on host %s\n", rank, hostname);


    if(mype == 0)
        std::cout << "Consumer - starting computation" << endl;

    std::string store_name = std::string(STORE_ID) + std::string("/") +std::to_string(*proc_id);
    st = nvs::StoreManager::GetInstance(store_name);
    nvs::init_log(nvs::SeverityLevel::all,"");

    // we have to read variables one by one. Bulk read make no sense...





    st->stats();

    MPI_Finalize();

    return 0;
}
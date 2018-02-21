
#include <cstdio>
#include <cmath>
#if defined(MPI_ENABLED)
#include <mpi.h>
#endif
#include <unistd.h>
#include <cstring>
#include <iostream>
#if defined(NV_STREAM)
#include "nvs/store.h"
#include "nvs/store_manager.h"
#include "nvs/log.h"
#endif


#define STORE_ID "micro_store"
#define N_ITER 10


void compute_kernel(double **array, long len, long row , long col){

    for(long i = 0; i < len; i ++) {
        double *mat_ary = array[i];
        double **d2_mat = (double **) mat_ary;
        for(int j =0; j < row; j++ ){
            for(int k =0; k < col; k++){
                d2_mat[j][k] = ((d2_mat[j][k] +1 )/1024);
            }
        }
    }
}

uint64_t compute_sum(double **ptr_array, long len){

    return 0;
}

uint64_t allocate_matrix(double **dptr, char name[10], long row, long col){
    //printf("row : %ld, col : %ld\n", row, col);
    double **var3;
    uint64_t size = row * sizeof(*var3) +
            row *(col * sizeof(**var3));

    void *ptr = malloc(size);

    double **twodmat = (double **)ptr;
    // format the array memory
    double *const data = (double *) (twodmat + row);
    int i,j;
    for(i=0;i < row; i++){
        twodmat[i] = data + i*col;
    }
    //populate matrix values
    for(i=0; i < row; i++){
        for(j=0; j < col; j++){
            twodmat[i][j] = i*col + (j+1);
        }
    }
    *dptr = (double *)ptr;
    return size;
}


uint64_t allocate_nvs_matrix(nvs::Store *store, double **dptr, char name[10], long row, long col){
    //printf("row : %ld, col : %ld\n", row, col);
    double **var3;
    int ret;
    uint64_t size = row * sizeof(*var3) +
                    row *(col * sizeof(**var3));

    void *ptr;
    ret = store->create_obj(name, size, &ptr);

    double **twodmat = (double **)ptr;
    // format the array memory
    double *const data = (double *) (twodmat + row);
    int i,j;
    for(i=0;i < row; i++){
        twodmat[i] = data + i*col;
    }
    //populate matrix values
    for(i=0; i < row; i++){
        for(j=0; j < col; j++){
            twodmat[i][j] = i*col + (j+1);
        }
    }
    *dptr = (double *)ptr;
    return size;
}




unsigned long tonum(char *ptr){

    char str[100];
    snprintf(str,100,"%s", ptr);
    char c = str[strlen(str) - 1];
    int temp = strlen(str);
    switch (c) {
        case 'k':
            str[temp - 1] = 0;
            return atol(str) * 1024;
        case 'm':
            str[temp - 1] = 0;
            return atol(str) * 1024 * 1024;
        case 'g':
            str[temp - 1] = 0;
            return atol(str) * 1024 * 1024 * 1024;
        default:
            return atol(str);
    }
}



int main (int argc, char *argv[]) {
    int mype, nproc;
    int N = 1000;
    double *ptr_array[N];
    int x;
    long total_var_size=0;
    long n_var=0;
    char c;
    char *varsizestr, *snapsizestr;
    int err;


    if(argc < 3) {
        printf("usage : <varible size> <snpshot budget>\n");
        return 1;
    }

    while((c = getopt(argc,argv,"v:s:")) != -1){
        switch (c){
            case 'v':
                varsizestr = optarg;
                break;
            case 's':
                snapsizestr = optarg;
                break;
            case '?':
                err = 1;
                break;
        }
    }



    long var_size = tonum(varsizestr);
    long snap_size = tonum(snapsizestr);

    printf("var size : %ld and snap size : %ld\n", var_size, snap_size);

    //var_size and snap_size are powers of two

    long mat_size = var_size/sizeof(double);
    double power = log2(mat_size);
    printf("mat size : %ld, power : %f\n", mat_size, power);
    long row = (long)power/2;
    long col = power - row;

    col = pow(2,col);
    row = pow(2,row);

#if defined(MPI_ENABLED)
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mype);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#else
    mype = 0;
#endif

    if(mype == 0)
        std::cout << "Producer - starting computation" << std::endl;

#if defined(NV_STREAM)
    std::string store_name = std::string(STORE_ID) + std::string("/") +std::to_string(mype);
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);
    nvs::init_log(nvs::SeverityLevel::all,"");

#endif

    /* number of objects that get created depends on the individual
     object size and checkpoint budget */

    int var_version=0;
    char var_name[10];


    do{
        if(n_var >  N){
            printf("not enough pointer array space length \n");
            exit(1);
        }

        snprintf(var_name,10,"v%d",var_version++);
#if defined(NV_STREAM)
        long alloc_size = allocate_nvs_matrix(st ,&ptr_array[n_var++],var_name,row,col);
#else
        long alloc_size = allocate_matrix(&ptr_array[n_var++],var_name,row,col);
#endif
        total_var_size += alloc_size;
        //printf("n_var : %ld, var_size : %ld , total_var_size : %ld \n",n_var ,var_size , total_var_size);

    }while(total_var_size < snap_size);

    printf("[%d] varibale size - %ld , snapshot budget - %ld \n",
           mype,var_size,snap_size);
for(int i =0; i < N_ITER; i ++) {
    // do computation
    compute_kernel(ptr_array,n_var,row,col);


    // snapshot
#if defined(NV_STREAM)
    st->put_all();
#endif

}
    //compute sum
    compute_sum(ptr_array,n_var);

    if(mype == 0){
        printf("sum of array across the ranks is : %ld \n", 0L);
    }

#if defined(MPI_ENABLED)
    MPI_Barrier(MPI_COMM_WORLD);
#endif
#if defined(NV_STREAM)
    st->stats();
#endif
#if defined(MPI_ENABLED)
    MPI_Finalize();
#endif

    return 0;

}

#include <stdio.h>
#include <mpi.h>

int main () {
    int mype, nproc;
    int N = 1000;
    int x;

    MPI_Init();
    MPI_Comm_rank(MPI_COMM_WORLD, &mype);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    // create object


    // do computation



    // write out



}
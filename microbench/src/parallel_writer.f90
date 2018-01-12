MODULE Global
    !we are going to  have twenty variables to simulate our apps
    real, pointer :: array_1d(:)


CONTAINS
    subroutine arrayincmod(array)
        real,pointer  :: array(:)
        do i = 1,size(array)
            array(i) = array(i) +1
            array(i)=MOD(array(i),100.0)
        end do
    end subroutine arrayincmod

END MODULE Global

program micro
    use Allocator
    use Global
    implicit none
    include 'mpif.h'

    real, pointer :: nstep
    integer, pointer :: istep
    CHARACTER(LEN=10) varname
    integer(4) :: length
    integer(4) :: cmtsize
    integer(4) :: fix_d
    integer(8):: nsize
    integer(4) :: ierr,mype,nproc


    call MPI_INIT(ierr)
    call MPI_COMM_SIZE(MPI_COMM_WORLD, nproc, ierr)
    call MPI_COMM_RANK(MPI_COMM_WORLD, mype, ierr)


    if(mype==0) print *, "Micro_C/R - Starting computation"
    call nvs_init(mype,nproc)

    nsize = 10
    call alloc_1d_real(array_1d, nsize, "array_1d", mype, nsize)
    !populate the array

    array_1d = 1

    call nvs_snapshot(mype)

    call nvs_finalize()


    call MPI_BARRIER(MPI_COMM_WORLD,ierr)
    call MPI_FINALIZE(ierr)

    if(mype == 0) write(0,*)'End of benchmark. Bye!!'
end program micro




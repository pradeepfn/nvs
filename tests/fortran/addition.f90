program main
 use Allocator
 implicit none
 include 'mpif.h'
 integer(4), parameter :: n = 10
 integer, pointer :: lcary(:)
 integer(4) :: a(n)
 integer(4) :: i, ista, iend, sum, ssum, ierr, mype, nproc
 CHARACTER(LEN=10) varname
 integer cmtsize;
 logical file_exist
 namelist /control/ irun
 integer irun

   inquire(file='add.input',exist=file_exist)
    if (file_exist) then
       open(55,file='add.input',status='old')
       read(55,nml=control)
       close(55)
    else
       write(0,*)'******************************************'
       write(0,*)'*** NOTE!!! Cannot find file add.input !!!'
       write(0,*)'*** Using default run parameters...'
       write(0,*)'******************************************'
    endif


 print *, irun 

 call MPI_INIT(ierr)
 call MPI_COMM_SIZE(MPI_COMM_WORLD, nproc, ierr)
 call MPI_COMM_RANK(MPI_COMM_WORLD, mype, ierr)
 call init(mype,nproc);

 call para_range(1, n, nproc, mype, ista, iend)

 varname = "lcary"
 cmtsize = iend-ista+1 
 call alloc_1d_int(lcary,cmtsize,varname, mype, cmtsize)

  do i = 1, (iend-ista+1)
   if(irun /= 1) then
      print *, "fresh start path"
      lcary(i) = i+(ista-1)
   else
      print *, "restart path"
      lcary(i) = lcary(i)+i+(ista-1)
   endif
  end do

 call MPI_BARRIER(MPI_COMM_WORLD,ierr)
 call chkpt_all(mype)

 sum = 0.0
 print *, lcary
 do i = 1, (iend-ista+1)
  sum = sum+ lcary(i)
 end do
 call MPI_REDUCE(sum, ssum, 1, MPI_INTEGER,MPI_SUM, 0,MPI_COMM_WORLD, ierr)
 sum = ssum
 if ( mype== 0 ) write(6,*)'sum =', sum
 call MPI_FINALIZE(ierr)




end program main
subroutine para_range(n1, n2, nprocs, irank, ista, iend)
integer(4) :: n1 ! Lowest value of iteration variable
integer(4) :: n2 ! Highest value of iteration variable
integer(4) :: nprocs ! # cores
integer(4) :: irank ! Iproc(rank)
integer(4) :: ista ! Start of iterations for rank iproc
integer(4) :: iend ! End of iterations for rank iproc
iwork1 = ( n2 -n1 + 1 ) / nprocs
iwork2 = MOD(n2 -n1 + 1, nprocs)
ista = irank * iwork1 + n1 + MIN(irank, iwork2)
iend = ista + iwork1 -1
if ( iwork2 > irank) iend = iend + 1
return
end subroutine para_range


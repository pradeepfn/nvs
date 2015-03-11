program checkpoint_write
    use Allocator

    real, pointer :: pw(:)
    real, pointer :: nstep
    integer, pointer :: istep
    CHARACTER(LEN=10) varname
    integer cmtsize
    integer __dummy
    integer i

    print *, "checkpointing"

    varname = "istep"
    call alloc_integer(istep,varname,1);
    istep = 5

    varname = "nstep"
    call alloc_real(nstep,varname,1);
    nstep = 7
 
    cmtsize = 10 
    varname = "pw"
    call alloc_1d_real(pw,cmtsize,varname,1,cmtsize) 

    !init array
    do i = 1, 10
       pw(i) = i*5
    end do
    
    call chkpt_all(1)
    do i = 1, 10
       print *, i, pw (i)
    end do
    print *, nstep
    print *, istep

end program checkpoint_write

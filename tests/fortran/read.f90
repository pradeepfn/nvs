program checkpoint_write
    use Allocator

    real, pointer :: pw(:)
    real, pointer :: nstep
    integer, pointer :: istep
    real, dimension(10) :: ar
    CHARACTER(LEN=10) varname
    integer cmtsize
    integer i

    print *, "restarting"

    varname = "nstep"
    call alloc_real(nstep,varname,1);

    cmtsize = 10 
    varname = "pw"
    call alloc_1d_real(pw,cmtsize,varname,1,cmtsize) 
    
    varname = "istep"
    call alloc_integer(istep,varname,1);

    do i = 1, 10
       print *, i, pw (i)
    end do
    print *, nstep
    print *, istep

end program checkpoint_write

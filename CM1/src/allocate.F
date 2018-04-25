module Allocator
    USE, INTRINSIC :: ISO_C_BINDING
    interface
        type(c_ptr) function alloc(vsize, varname) bind(C)
            USE, INTRINSIC :: ISO_C_BINDING
            integer(KIND=C_LONG) :: vsize
            character(KIND=C_CHAR) :: varname(10)
            !integer(KIND=C_INT) :: pid, cmtsize
        end function alloc
    end interface

contains
    subroutine alloc_1d_real(arr,row,varname, mype, cmtsize)
        integer*8 :: row
        real, pointer :: arr(:)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname
        integer*8 :: cmtsize
        integer :: mype

        val = row*SIZEOF(real)
        nvsize = cmtsize * SIZEOF(real)
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row])
    end subroutine

    subroutine alloc_1d_real8(arr,row,varname, mype, cmtsize)
        integer :: row
        real*8, pointer :: arr(:)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname
        integer*8 :: cmtsize
        integer :: mype 
        !sizeof(real*8) is hard coded as 8. expression didnt work
        val = row * 8
        nvsize = cmtsize * 8
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row])
    end subroutine



    subroutine alloc_2d_real(arr,row,col,varname, mype, cmtsize)
        integer :: row
        integer :: col
        integer*8 :: cmtsize
        integer :: mype
        real, pointer :: arr(:,:)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname

        val = row*col*SIZEOF(real)
        nvsize = cmtsize * SIZEOF(real)
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row,col])
    end subroutine

    subroutine alloc_2d_int(arr,row,col,varname, mype, cmtsize)
        integer :: row
        integer :: col
        integer*8 :: cmtsize
        integer :: mype
        integer, pointer :: arr(:,:)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname

        val = row*col*SIZEOF(real)
        nvsize = cmtsize * SIZEOF(real)
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row,col])
    end subroutine


    subroutine alloc_3d_int(arr,row,col,z,varname, mype, cmtsize)
        integer*8 :: row
        integer*8 :: col
        integer*8 :: z
        integer*8 :: cmtsize
        integer :: mype
        integer, pointer :: arr(:,:, :)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname

        val = row*col* z*SIZEOF(real)
        nvsize = cmtsize * SIZEOF(real)
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row,col,z])
    end subroutine



    subroutine alloc_3d_real(arr,row,col,z,varname, mype, cmtsize)
        integer :: row
        integer :: col
        integer :: z
        integer :: mype
        integer*8 :: cmtsize
        real, pointer :: arr(:,:,:)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname

        nvsize = cmtsize * SIZEOF(real)
        val = row*col*z*SIZEOF(real)
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row,col,z])
    end subroutine

    subroutine alloc_4d_real(arr,row,col,z,k,varname, mype, cmtsize)
        integer :: row
        integer :: col
        integer :: z
        integer :: k
        integer :: mype
        integer*8 :: cmtsize
        real, pointer :: arr(:,:,:,:)
        type(c_ptr) :: cptr
        integer*8 :: val, nvsize
        character(len=10) varname

        nvsize = cmtsize * SIZEOF(real)
        val = row*col*z*k*SIZEOF(real)
        cptr = alloc(val,varname)
        call c_f_pointer(cptr,arr,[row,col,z,k])
    end subroutine
end module

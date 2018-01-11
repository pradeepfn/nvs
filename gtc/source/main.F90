!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!                                                                            !
!                 Gyrokinetic Toroidal Code (GTC)                            !
!                          Version 2                                         !
!          Zhihong Lin, Stephane Ethier, Jerome Lewandowski                  !
!              Princeton Plasma Physics Laboratory                           !
!                                                                            !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

program gtc
  use global_parameters
  use particle_array
  use particle_tracking
  use field_array
  use diagnosis_array
  implicit none

  integer i,ierror
  real(doubleprec) time(9),timewc(9),t0,dt,t0wc,dtwc,loop_time
  real(doubleprec) tracktcpu,tracktwc,tr0,tr0wc
  character(len=10) ic(8)

#ifdef __NERSC
! NERSC USG utilization statistics. This routine is available only on the
! NERSC computers so it is included at the preprocessing stage only if
! the variable __NERSC is defined (see the Makefile).
!  call system_stats()
#endif

! MPI initialize
  call mpi_init(ierror)

  tr0=0.0
  tr0wc=0.0
  tracktcpu=0.0
  tracktwc=0.0

  time=0.0
  t0=0.0
  timewc=0.0
  t0wc=0.0
  call timer(t0,dt,t0wc,dtwc)
  time(8)=t0
  timewc(8)=t0wc
  istep=0

! input parameters, setup equilibrium, allocate memory 
  CALL SETUP

! initialize particle position and velocity
  CALL LOAD
! Write out initial position of tracked particles
!!!  if(track_particles==1)then
!!!     call locate_tracked_particles
!!!     call write_tracked_particles
!!!  endif

  CALL CHARGEI !calculate ion gather-scatter coefficients
  call timer(t0,dt,t0wc,dtwc)
  time(7)=time(7)+dt
  timewc(7)=timewc(7)+dtwc
  loop_time=t0wc

! main time loop
  do istep=1,mstep
     do irk=1,2

! idiag=0: do time history diagnosis
        idiag=mod(irk+1,2)+mod(istep,ndiag)

! smooth potential, diagnostics
        CALL SMOOTH(3)
        call timer(t0,dt,t0wc,dtwc)
        time(5)=time(5)+dt
        timewc(5)=timewc(5)+dtwc

! field
        CALL FIELD
        call timer(t0,dt,t0wc,dtwc)
        time(6)=time(6)+dt
        timewc(6)=timewc(6)+dtwc

! push ion
        CALL PUSHI
        if(idiag==0)then
          CALL DIAGNOSIS
        !!!  CALL VOLUME    !Original netCDF 3D potential data
        !!!  CALL OUTPUT3D  !HDF5 parallel output of 3D potential data
        !!!  CALL DATAOUT   !New version of netCDF 3D potential data
        endif
        call timer(t0,dt,t0wc,dtwc)
        time(1)=time(1)+dt
        timewc(1)=timewc(1)+dtwc
        
! redistribute ion across PEs
        CALL SHIFTI
        call timer(t0,dt,t0wc,dtwc)
        time(2)=time(2)+dt
        timewc(2)=timewc(2)+dtwc

! collisions
        !if(irk==2 .and. do_collision)CALL LORENTZ_COLL
        if(irk==2 .and. do_collision)CALL collision
        call timer(t0,dt,t0wc,dtwc)
        time(9)=time(9)+dt
        timewc(9)=timewc(9)+dtwc

! ion perturbed density
        CALL CHARGEI
        call timer(t0,dt,t0wc,dtwc)
        time(3)=time(3)+dt
        timewc(3)=timewc(3)+dtwc

! smooth ion density
        CALL SMOOTH(0)
        call timer(t0,dt,t0wc,dtwc)
        time(5)=time(5)+dt
        timewc(5)=timewc(5)+dtwc

! solve GK Poisson equation using adiabatic electron
        CALL POISSON(0)
        call timer(t0,dt,t0wc,dtwc)
        time(4)=time(4)+dt
        timewc(4)=timewc(4)+dtwc

        do ihybrid=1,nhybrid
! smooth potential
           CALL SMOOTH(2)
           call timer(t0,dt,t0wc,dtwc)
           time(5)=time(5)+dt
           timewc(5)=timewc(5)+dtwc

! push electron, sub-cycling
           do i=1,ncycle*irk
! 1st RK step
              CALL PUSHE(i,1)
              call timer(t0,dt,t0wc,dtwc)
              time(1)=time(1)+dt
              timewc(1)=timewc(1)+dtwc

              CALL SHIFTE
              call timer(t0,dt,t0wc,dtwc)
              time(2)=time(2)+dt
              timewc(2)=timewc(2)+dtwc

! 2nd RK step
              CALL PUSHE(i,2)
              call timer(t0,dt,t0wc,dtwc)
              time(1)=time(1)+dt
              timewc(1)=timewc(1)+dtwc

              CALL SHIFTE
              call timer(t0,dt,t0wc,dtwc)
              time(2)=time(2)+dt
              timewc(2)=timewc(2)+dtwc
           enddo

! nonadiabatic electron charge density
           CALL CHARGEE
           call timer(t0,dt,t0wc,dtwc)
           time(3)=time(3)+dt
           timewc(3)=timewc(3)+dtwc

! smooth electron density
           CALL SMOOTH(1)
           call timer(t0,dt,t0wc,dtwc)
           time(5)=time(5)+dt
           timewc(5)=timewc(5)+dtwc

! solve GK Poisson equation using non-adiabatic electron
           CALL POISSON(1)
           call timer(t0,dt,t0wc,dtwc)
           time(4)=time(4)+dt
           timewc(4)=timewc(4)+dtwc
        enddo
     enddo

     call timer(tr0,dt,tr0wc,dtwc)
     if(track_particles==1 .and. nptrack>0)call locate_tracked_particles
     call timer(tr0,dt,tr0wc,dtwc)
     tracktcpu=tracktcpu+dt
     tracktwc=tracktwc+dtwc

! profile snapshots, write particle information to restart file
        CALL SNAPSHOT
     if(mod(istep,mstep/msnap) .eq. 0)then
        !CALL SNAPSHOT
        if(track_particles==1 .and. nptrack>0)call write_tracked_particles
     endif
  enddo

  call timer(t0,dt,t0wc,dtwc)
  loop_time=t0wc-loop_time
  time(8)=t0-time(8)
  timewc(8)=t0wc-timewc(8)
  ic(1)='pusher'
  ic(2)='shift'
  ic(3)='charge'
  ic(4)='poisson'
  ic(5)='smooth'
  ic(6)='field'
  ic(7)='load'
  ic(8)='total'
  if(mype==0)then
     write(stdout,*)'CPU TIME USAGE (in SEC):'
     write(stdout,*)ic
     write(stdout,'(8(1pe10.3),/)')time(1:8)
     write(stdout,*)'WALL CLOCK TIMES (in SEC):'
     write(stdout,*)ic
     write(stdout,'(8(1pe10.3))')timewc(1:8)
     write(stdout,'("TOTAL CPU TIME USAGE (SEC):",f12.3)')time(8)
     write(stdout,'("TOTAL WALL CLOCK TIME(SEC):",f12.3)')timewc(8)
     write(stdout,'("CPU+WALL TIMES IN COLL(SEC):",2f12.3)')time(9),timewc(9)
     write(stdout,'("NERSC TIME(SEC):",f12.3)')loop_time
     if(track_particles==1)write(stdout,'("PARTICLE TRACKING TIME(SEC):",f12.3)')tracktwc
  endif

#ifdef __NERSC
!  call system_stats()
#endif
#ifdef _YUMA
  call px_finalize()

#endif

! MPI finalize
  call mpi_finalize(ierror)

end program gtc

!=========================================
subroutine timer(t0,dt,t0wc,dtwc)
!=========================================
  use precision
  implicit none
  real(doubleprec) t0,dt,t0wc,dtwc
  real(doubleprec) t1,t1wc

! Get cpu usage time since the beginning of the run and subtract value
! from the previous call
  call cpu_time(t1)
  dt=t1-t0
  t0=t1

! Get wall clock time and subtract value from the previous call
  t1wc=MPI_WTIME()
  dtwc=t1wc-t0wc
  t0wc=t1wc

end subroutine timer


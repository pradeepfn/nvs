SUBROUTINE Lorentz_coll

  use global_parameters
  use particle_array
  use rng_gtc
  implicit none

  real(wp) vthi,aion_inv,r,b,vpara,vperp2,v,nudt,ran_num,get_random_number,&
           nudt_vth,sff,pitch,panew,unitf
  integer  m,i,iss
 
!   C_{pitch-angle}:  pitch angle scattering collisions

  aion_inv=1.0_wp/aion
  vthi=gyroradius*abs(qion)*aion_inv
  nudt_vth=tstep/tauii  ! Probability of collision for a particle of velocity
                        ! vthi during one simulation time step (tstep)

  do m=1,mi
     r=sqrt(2.0_wp*zion(1,m))
     b=1.0_wp/(1.0_wp+r*cos(zion(2,m)))
     vpara=zion(4,m)*b*qion*aion_inv
     vperp2=2.0_wp*aion_inv*b*zion(6,m)*zion(6,m)
     v=sqrt(vperp2+vpara*vpara)

   ! We now calculate nudt, the effective collision probability.
   ! Here we neglect the small density variation due to delta_f.
   ! In our simulations, the background density is constant everywhere.
     nudt=(vthi/v)**3*nudt_vth

   ! For particles with a very small velocity the collision probability
   ! calculated with the current time step may be too large to be physical.
   ! If this is the case, we subdivide the time step and loop over the
   ! number of subdivisions.
     sff=0.25_wp
     iss=max(1,min(int(nudt/sff),100000))  ! number of subdivisions
     nudt=nudt/real(iss,wp)  ! We scale down the collision probability
     pitch=vpara/v
     do i=1,iss
       ran_num=get_random_number()
       unitf=sign(1.0_wp,(ran_num-0.5_wp))
       panew=max(-1.0_wp,min(1.0_wp,pitch*(1.0_wp-nudt)+&
                      unitf*sqrt((1.0_wp-pitch**2)*nudt) ))
       pitch=panew
     end do
     vpara=panew*v

   ! Update particle velocity-related quantities (rho_parallel and sqrt(mu))
     zion(4,m)=vpara*aion/(b*qion)
     zion(6,m)=sqrt(0.5_wp*aion*(v*v-vpara*vpara)/b)

  end do

end subroutine Lorentz_coll

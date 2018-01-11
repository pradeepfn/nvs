#!/bin/bash
#make restartclean
rm -rf stats/.*	
#mpiexec -n 16 -hostfile host_file ./gtc 
mpirun -np 64 --bind-to core ../../phoenix/bin/mpiformat /dev/shm 1000
mpirun -np 64 --bind-to core ./gtcmpi 


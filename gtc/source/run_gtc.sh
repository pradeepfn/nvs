#!/bin/bash
rm -rf /dev/shm/unity_NVS_ROOT*
rm -rf /dev/shm/shm
mpirun -np 1 --bind-to core /home/pradeep/nvs/nvstream/build/src/tools/nvsformat
rm -rf stats/.*	
#mpiexec -n 16 -hostfile host_file ./gtc 
mpirun -np 4 --bind-to core ./gtcmpi 


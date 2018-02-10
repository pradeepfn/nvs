#!/bin/bash
rm -rf /dev/shm/unity_NVS_ROOT*
rm -rf /dev/shm/shm
mpirun -np 1 --bind-to core ../../nvstream/build/src/tools/nvsformat
rm -rf stats/.*	
mpirun -np 4 --bind-to core ./gtcmpi 


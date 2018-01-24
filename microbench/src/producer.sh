#!/bin/bash
rm -rf /dev/shm/unity_NVS_ROOT*
rm -rf /dev/shm/shm
mpirun -np 1 --bind-to core ../../nvstream/build/src/tools/nvsformat
mpirun -np 4 --bind-to core ./p_writer

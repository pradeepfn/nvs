#!/bin/bash
rm -rf /dev/shm/unity_NVS_ROOT*
rm -rf /dev/shm/shm
mpirun -np 1 --bind-to core ../../nvstream/build/src/tools/nvsformat
mpirun -np 1 --bind-to core ./micro_writer -v 1k -s 100k

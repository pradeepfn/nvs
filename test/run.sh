#!/bin/bash
rm -rf /dev/shm/mmap*
mpirun -np 4 ./mpiformat /dev/shm 100

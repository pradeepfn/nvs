#!/bin/bash
rm -rf /dev/shm/unity
rm -rf stats/.*	
mpirun -np 4 --bind-to core ./gtcmpi 


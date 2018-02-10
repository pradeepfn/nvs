#!/bin/bash
rm -rf /mnt/pmfs/unity
rm -rf stats/.*	
mpirun -np 4 --bind-to core ./gtcmpi 


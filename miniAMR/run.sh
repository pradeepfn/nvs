#!/bin/bash

mpirun -np 16 --bind-to core ../phoenix/bin/mpiformat /dev/shm 500
mpirun -np 16 -bind-to core ./miniAMR.x --num_refine 4 --max_blocks 4000 --init_x 1 --init_y 1 --init_z 1 --npx 4 --npy 2 --npz 2 --nx 8 --ny 8 --nz 8 --num_objects 2 --object 2 0 -1.10 -1.10 -1.10 0.030 0.030 0.030 1.5 1.5 1.5 0.0 0.0 0.0 --object 2 0 0.5 0.5 1.76 0.0 0.0 -0.025 0.75 0.75 0.75 0.0 0.0 0.0 --num_tsteps 30 --checksum_freq 4 --stages_per_ts 16

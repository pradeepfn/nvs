#!/usr/bin/env bash

#declare -a store=("tmpfs" "pmfs" "memcpy" "nvs" "dnvs")
declare -a store=("tmpfs")
declare -a thr=(4)

for st in "${store[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$st"

	cd ..
	#build the nvstream lib to the given store type
	./build_nvs.py -c 
	./build_nvs.py -b -s "$st"

	#build amr application
	cd miniAMR
	make clean
	make
	for th in "${thr[@]}"
	do
		#remove data from previous runs
		rm -rf /dev/shm/unity_NVS_ROOT*
		rm -rf /dev/shm/shm
		rm -rf /dev/shm/unity
		if [ "$st" == "pmfs" ]
		then
			rm -rf /mnt/pmfs/unity
		fi

		#single node run
		mpirun -np 1 --bind-to core ../nvstream/build/src/tools/nvsformat

		#clean up stats directory in gtc source directory
		rm -rf stats
		mkdir stats


		mpirun -np "$th" -bind-to core ./miniAMR.x \
			--num_refine 4 --max_blocks 4000 --init_x 1 --init_y 1 --init_z 1 \
			--npx 4 --npy     2 --npz 2 --nx 8 --ny 8 --nz 8 --num_objects 2 \
			--object 2 0 -1.10 -1.10 -1.10 0.030 0.030 0.030 1.5 1.5 1.5 0.0 0.0 0.0 \
			--object 2 0 0.5 0.5 1.76 0.0 0.0 -0.025 0.75 0.75 0.75 0.0 0.0 0.0 \
			--num_tsteps 30 --checksum_freq 4 --stages_per_ts 16


		> ../results/"$st"/amr_"$st"_t"$th".txt 2>&1
		mkdir -p ../results/"$st"/amr_"$st"_t"$th" 
		cp -a stats/. ../results/"$st"/amr_"$st"_t"$th"/
		#rm -rf stats
	done
done
exit

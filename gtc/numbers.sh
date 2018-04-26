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

	#build gtc application
	cd gtc/source
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
		mpirun -np 1 --bind-to core ../../nvstream/build/src/tools/nvsformat

		#clean up stats directory in gtc source directory
		rm -rf stats
		mkdir stats
		mpirun -np "$th" --bind-to core ./gtcmpi > ../results/"$st"/gtc_"$st"_t"$th".txt 2>&1
		mkdir -p ../results/"$st"/gtc_"$st"_t"$th" 
		cp -a stats/. ../results/"$st"/gtc_"$st"_t"$th"/
		#rm -rf stats
	done
	cd ..
done
exit

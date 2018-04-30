#!/usr/bin/env bash

#declare -a store=("tmpfs" "pmfs" "memcpy" "nvs" "dnvs")
declare -a store=("nvs")
# this should match the no of processors pass in to to mpirun command
node_x=4
node_y=1
declare -a thr=(4)

restart_freq=10

echo $node_x
echo $node_y

cd run
cp namelist.input.orig namelist.input

sed -i "s/nodex        =       1/nodex =  $node_x/" namelist.input
sed -i "s/nodey        =       1/nodey =  $node_y/" namelist.input
sed -i "s/rstfrq = -3600.0/rstfrq = $restart_freq/" namelist.input
cd ..

for st in "${store[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$st"

	cd ..
	#build the nvstream lib to the given store type
	./build_nvs.py -c
	./build_nvs.py -b -s "$st"

	#build gtc application
	cd CM1/src
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

		cd ../run
		#clean up stats directory in CM1 run directory
		rm -rf stats
		mkdir stats

		#mpirun -np "$th" --bind-to core ./gtcmpi > ../results/"$st"/gtc_"$st"_t"$th".txt 2>&1
		mpirun -np "$th" --bind-to core ./cm1.exe
		mkdir -p ../results/"$st"/cm1_"$st"_t"$th"
		cp -a stats/. ../results/"$st"/cm1_"$st"_t"$th"/

	done
	cd ..
done
exit




#!/usr/bin/env bash
#declare -a store=("memcpy" "tmpfs" "pmfs" "nvs" "dnvs")
declare -a store=("nvs")
declare -a varsize=("1k" "64k" "128k" "512k" "1m" "5m" "10m")
#declare -a varsize=("50m")

snapsize=20m

for st in "${store[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$st"

	cd ..
	#build the nvstream lib to given store type
	./build_nvs.py -c 
	./build_nvs.py -b -s "$st"

	cd microbench
	for vs in "${varsize[@]}"
	do

		#remove data from previous runs
		rm -rf /dev/shm/unity_NVS_ROOT*
		rm -rf /dev/shm/shm
		rm -rf /dev/shm/unity
		if [ "$st" == "pmfs" ]
		then
			rm -rf /mnt/pmfs/unity
		fi
		
		 mpirun -np 1 --bind-to core ../nvstream/build/src/tools/nvsformat
	
		 #clean up the stats directory
		 rm -rf stats
		 mkdir stats

		./micro_bench.py -c -w "$st"
		./micro_bench.py -b -w "$st"
		./micro_bench.py -r -w "$st" -v "$vs" -s "$snapsize"> results/"$st"/micro_"$bm"_v"$vs"_s"$snapsize".txt
		 mkdir -p results/"$st"/micro_"$st"_v"$vs"
		 cp -a stats/.  results/"$st"/micro_"$st"_v"$vs"/
	done
done
exit




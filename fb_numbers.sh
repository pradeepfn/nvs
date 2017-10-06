#!/usr/bin/env bash
#declare -a bname=("bigfile")
declare -a bname=("mmapnoflush")
declare -a chunksize=("256" "1k" "4k" "16k" "64k" "256k" "1m")
#declare -a chunksize=("1m")
filesize=16g
stepsize=1m

for bm in "${bname[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$bm"
	for cs in "${chunksize[@]}"
	do
		./run_bench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"  > results/"$bm"/"$bm"_f"$filesize"_s"$stepsize"_c"$cs".txt
		#./run_bench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"
		echo "removing the benchmark files"
		rm -rf /mnt/pmfs/*
		sleep 5
	done
done
exit




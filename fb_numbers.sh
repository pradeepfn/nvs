#!/usr/bin/env bash
#declare -a bname=("newfile" "bigfile")
declare -a bname=("mmap")
declare -a chunksize=("64" "256" "1k" "4k" "16k" "64k" "256k" "1m")
#declare -a chunksize=("64k")
filesize=4g
stepsize=1m
rep=5

for bm in "${bname[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$bm"
	for cs in "${chunksize[@]}"
	do
		./run_bench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"  > results/"$bm"/"$bm"_f"$filesize"_s"$stepsize"_c"$cs".txt
	done
done
exit




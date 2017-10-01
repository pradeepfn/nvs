#!/usr/bin/env bash
declare -a bname=("newfile" "bigfile")
declare -a chunksize=("64k" "128k" "256k")
#declare -a chunksize=("64k")
filesize=1g
stepsize=1m
rep=5

for bm in "${bname[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$bm"
	for cs in "${chunksize[@]}"
	do
		./run_fbench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"  > results/"$bm"/"$bm"_f"$filesize"_s"$stepsize"_c"$cs".txt
	done
done
exit




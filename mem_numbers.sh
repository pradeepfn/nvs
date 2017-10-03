#!/usr/bin/env bash
#declare -a stepsize=("1m")
declare -a chunksize=("64k" "128k" "256k")
stepsize=1m
rep=5
#create output dir if not exist
mkdir -p results/memmap
for cs in "${chunksize[@]}"
do
	./mmapbench/mmapbench -s "$stepsize" -c "$cs" > results/memmap/memmap_s"$stepsize"_c"$cs".txt
done
exit




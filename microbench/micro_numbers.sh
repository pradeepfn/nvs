#!/usr/bin/env bash
declare -a bname=("nvs" "pmfs" "tmpfs")
#declare -a varsize=("1k" "64k" "512k" "1m" "10m" "50m" "100m")
declare -a varsize=("1k")
snapsize=100k

for bm in "${bname[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$bm"
	for cs in "${varsize[@]}"
	do
		./micro_bench.py -c -w "$bm"
		./micro_bench.py -b -w "$bm"
		./micro_bench.py -w "$bm" -v "$varsize" -s "$snapsize"> results/"$bm"/"$bm"_v"$varsize"_s"$snapsize".txt
	done
done
exit




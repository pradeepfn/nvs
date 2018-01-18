#!/usr/bin/env bash
declare -a bname=("tmpfsnewfile" "tmpfsbigfile")
#declare -a bname=("mmapnoflush")
#declare -a bname=("mmapstream")
declare -a chunksize=("256" "1k" "4k" "16k" "64k" "256k" "1m")
#declare -a chunksize=("256")
#declare -a chunksize=("1m")
filesize=16g
stepsize=1m


rep=2


for bm in "${bname[@]}"
do
	#create output dir if not exist
	mkdir -p results/"$bm"
	for cs in "${chunksize[@]}"
	do
		./run_bench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"  > results/"$bm"/"$bm"_f"$filesize"_s"$stepsize"_c"$cs".txt
		#./run_bench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"
		echo "removing the benchmark files"
		#rm -rf /mnt/pmfs/*
		rm -rf /dev/shm/*
		sleep 5
           for i in `seq 2 $rep`
		do
			./run_bench.py -w "$bm" -t "$filesize" -s "$stepsize" -c "$cs"  >> results/"$bm"/"$bm"_f"$filesize"_s"$stepsize"_c"$cs".txt
			#rm -rf /mnt/pmfs/*
			rm -rf /dev/shm/*
			sleep 5
		done

	done
done
exit




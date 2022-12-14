#!/bin/bash

RESULT_DIR=results
BENCH_TYPE=$1

intervals=(2000 5000 10000 20000 40000 100000)
sizes=(64 256 1024 4096 16384 32768 65536)
#sizes=(65536)
for interval in "${intervals[@]}"
do
	for size in "${sizes[@]}"
	do
		./run_benchmark.sh -m $interval -s $size -o $RESULT_DIR/${BENCH_TYPE}_m${interval}_s${size}.results $BENCH_TYPE
	done
done

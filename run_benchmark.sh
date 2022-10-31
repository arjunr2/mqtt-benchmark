#!/bin/bash

shopt -s expand_aliases
source ~/.alias

ADDRESS=hc-00.arena.andrew.cmu.edu
NAME=\`uuidgen\`
TOPIC=\`uuidgen\`
INTERVAL=100000
ITER=100
DROP_RATIO=10
QOS=0
LOG=""
SIZE=64

while getopts 'a:n:m:i:q:s:d:t:vo:h' opt
do
	case "${opt}" in
		a) ADDRESS=$OPTARG;;
		n) NAME=$OPTARG;;
		m) INTERVAL=$OPTARG;;
		i) ITER=$OPTARG;;
		q) QOS=$OPTARG;;
		s) SIZE=$OPTARG;;
		d) DROP_RATIO=$OPTARG;;
		t) TOPIC=$OPTARG;;
		v) LOG="--log";;
		o) OUTPUT_FILE=$OPTARG;;
		h) echo "Usage: ./run_benchmark.sh [-anmiqsdtvo] BENCH_TYPE"
		   exit 1;;
	esac
done
BENCH_TYPE=${@:$OPTIND:1}

OUTPUT_FILE="${OUTPUT_FILE:-$BENCH_TYPE.results}"
echo "Writing results to $OUTPUT_FILE"

script_str="cd mqtt-benchmark; ./benchmark --address=$ADDRESS --name=$NAME --interval=$INTERVAL --iterations=$ITER --topic=$TOPIC --drop-ratio=$DROP_RATIO --qos=$QOS --size=$SIZE $LOG | python3 postprocess.py"
echo $script_str


OUT=""
case $BENCH_TYPE in
	nointerference)
		OUT=$(hc cmd -x "$script_str") ;;
	isolated)
		OUT=$(hc cmd --sync -x "$script_str") ;;
	*)
		echo "Invalid benchtype (options: nointerference | isolated)"
		exit 1
		;;
esac

echo "$OUTPUT_FILE | Interval=$INTERVAL ; Size=$SIZE; Iter=$ITER ; QOS=$QOS ; Drop=$DROP_RATIO" > $OUTPUT_FILE
echo "$OUT" | tee log \
	  | awk '/\[hc-[0-9]+\]/ {print $1 nr[NR+15] nr[NR+16] nr[NR+17];next}; NR in nr' \
	  | sed "s/.*/&,/" \
	  | xargs -d"\n" -n3	\
	  | column -t -s ","	\
	  | sort \
	  >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE

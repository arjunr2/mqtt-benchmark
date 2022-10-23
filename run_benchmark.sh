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

OUT=""

while getopts 'a:n:m:i:q:s:d:t:v' opt
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
	esac
done
BENCH_TYPE=${@:$OPTIND:1}

script_str="cd mqtt-benchmark; ./benchmark --address=$ADDRESS --name=$NAME --interval=$INTERVAL --iterations=$ITER --topic=$TOPIC --drop-ratio=$DROP_RATIO --qos=$QOS --size=$SIZE $LOG"
echo $script_str


case $BENCH_TYPE in
	nointerference)
		OUT=$(hc cmd -x "$script_str") ;;
	isolated)
		OUT=$(hc cmd --sync -x "$script_str") ;;
	*)
		echo "Invalid argument (options: nointerference | isolated)"
		exit 1
		;;
esac

echo "$1 | Interval=$INTERVAL ; Iter=$ITER ; QOS=$QOS ; Drop=$DROP_RATIO" > $1.results  
echo "$OUT" | tee log \
	  | awk '/\[hc-[0-9]+\]/ {print $1 nr[NR+15] nr[NR+16];next}; NR in nr' \
	  | sed "s/.*/&,/" \
	  | xargs -d"\n" -n3	\
	  | column -t -s ","	\
	  | sort \
	  >> $1.results

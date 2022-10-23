#!/bin/bash

#hc cmd --devices hc-10 -x "cd mqtt-benchmark; ./benchmark --address=hc-00.arena.andrew.cmu.edu --name=\`uuidgen\` --interval=100000 --iterations=100 --topic=\`uuidgen\` --drop-ratio=10"

ADDRESS=hc-00.arena.andrew.cmu.edu
NAME=\`uuidgen\`
TOPIC=\`uuidgen\`
INTERVAL=100000
ITER=100
DROP_RATIO=10
QOS=0

OUT=""

script_str="cd mqtt-benchmark; ./benchmark --address=$ADDRESS --name=$NAME --interval=$INTERVAL --iterations=$ITER --topic=$TOPIC --drop-ratio=$DROP_RATIO --qos=$QOS"

case $1 in
	nointerference)
		OUT=$(hc cmd -x "$script_str")
		;;
	isolated)
		OUT=$(hc cmd --sync -x "$script_str")
		;;
	*)
		echo "Invalid argument (options: nointerference | isolated)"
		exit 1
		;;
esac

echo "$1 ; Interval=$INTERVAL ; Iter=$ITER ; QOS=$QOS ; Drop=$DROP_RATIO" > $1.results  
echo "$OUT" | tee log \
	  | awk '/\[hc-[0-9]+\]/ {print $1 nr[NR+15] nr[NR+16];next}; NR in nr' \
	  | sed "s/.*/&,/" \
	  | xargs -d"\n" -n3	\
	  | column -t -s ","	\
	  | sort \
	  >> $1.results

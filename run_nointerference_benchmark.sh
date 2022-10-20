#!/bin/bash
hc cmd -x "cd mqtt-benchmark; ./benchmark --address=hc-00.arena.andrew.cmu.edu --name=\`uuidgen\` --interval=100000 --iterations=100 --topic=\`uuidgen\` --drop-ratio=10" \
	| tee log \
	| awk '/\[hc-[0-9]+\]/ {printf "%s | ",$1 nr[NR+15]; next}; NR in nr' \
	| sort \
	> nointerference_results

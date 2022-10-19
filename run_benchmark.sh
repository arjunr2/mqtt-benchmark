#!/bin/bash
hc cmd --devices hc-10 -x "cd mqtt-benchmark; ./benchmark --address=hc-00.arena.andrew.cmu.edu --name=\`uuidgen\` --interval=100000 --iterations=100 --topic=\`uuidgen\` --drop-ratio=10"

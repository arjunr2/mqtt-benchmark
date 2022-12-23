#!/bin/bash

shopt -s expand_aliases
source ~/.alias

git pull
hc cmd -x "mkdir -p mqtt-benchmark"
hc cmd --action=put --src mqtt.c --dst mqtt-benchmark/mqtt.c
hc cmd --action=put --src Makefile --dst mqtt-benchmark/Makefile
hc cmd -x "cd mqtt-benchmark; make"

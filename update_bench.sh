#!/bin/bash

shopt -s expand_aliases
source ~/.alias

hc cmd -x "[ ! -d \"mqtt-benchmark\" ] && git clone https://github.com/arjunr2/mqtt-benchmark.git; cd mqtt-benchmark; git pull; make"

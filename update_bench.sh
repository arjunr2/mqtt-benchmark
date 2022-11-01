#!/bin/bash

shopt -s expand_aliases
source ~/.alias

hc cmd -x "cd mqtt-benchmark; git pull; make"

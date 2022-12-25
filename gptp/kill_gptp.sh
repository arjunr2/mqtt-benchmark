#!/bin/bash

shopt -s expand_aliases
source ~/.alias

hc cmd --devices hc-33 hc-34 hc-35 --action sudo --password wiselab2022 -x "pkill -f ptp4l" 
hc cmd --devices hc-33 hc-34 hc-35 --action sudo --password wiselab2022 -x "pkill -f phc2sys" 


#!/bin/bash

shopt -s expand_aliases
source ~/.alias
source gptp_env.sh

hc cmd --devices $GM_NODE $FW_NODES --action sudo --password wiselab2022 -x "pkill -f ptp4l" 
hc cmd --devices $GM_NODE $FW_NODES --action sudo --password wiselab2022 -x "pkill -f phc2sys" 


#!/bin/bash

shopt -s expand_aliases
source ~/.alias
source ptp_env.sh

hc cmd --devices $GM_NODE --action sudo --password wiselab2022 -x "./$GPTP_DIR/run_gptp_master.sh" 
hc cmd --devices $FW_NODES --action sudo --password wiselab2022 -x "./$GPTP_DIR/run_gptp_client.sh" 


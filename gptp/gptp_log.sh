#!/bin/bash

shopt -s expand_aliases
source ~/.alias
source gptp_env.sh

echo "--- PTP LOGS ---"
hc cmd --devices $GM_NODE $FW_NODES --action sudo --password wiselab2022 -x "tail -n6 $GPTP_DIR/ptp.log" 
echo "----------------\n"
echo "--- PHC2SYS LOGS ---"
hc cmd --devices $GM_NODE $FW_NODES --action sudo --password wiselab2022 -x "tail -n6 ./$GPTP_DIR/phc2sys.log" 
echo "--------------------\n"


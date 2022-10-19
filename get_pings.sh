#!/bin/bash
hc cmd -x "ping -c 8 hc-00.arena.andrew.cmu.edu | tail -1 " > pings

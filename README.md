# MQTT Network Benchmarking

Estimating pubsub network characteristics such as propagation and queuing delays are hard to model mathematically for distributed systems. This repository contains code to benchmark MQTT network characteristics (currently only latency) easily across custom communication topologies in a distributed cluster.

##  Setup
 
### PTP Time Synchronization

**NOTE**: PTP is only required if you are running any inter-node latency tests. Round-trip tests do not require PTP

The [gptp](gptp) directory contains scripts for time synchronization. 

Hardware timestamping is required on all nodes to be synced. The switch must support TSN standards and enable *gPTP* (IEEE 802.1AS) on all the ports. Set the Grandmaster node (GM) and Follower nodes (FW) in the (gptp_env)[gptp/gptp_env.sh]. Execute `run_ptp.sh` to synchronize nodes and `kill_ptp.sh` to end synchronization. To simplify the process, you can run the following makefile target in the base directory:

```shell
make time-sync
```

It can take up to a minute for node system time to stabilize. Look at the  

### 

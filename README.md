# MQTT Network Benchmarking

Estimating pubsub network characteristics such as propagation and queuing delays are hard to model mathematically for distributed systems. This repository contains code to benchmark MQTT network characteristics (currently only latency) easily across custom communication topologies in a distributed cluster.

##  Setup
 
### PTP Time Synchronization

**NOTE**: PTP is only required if you are running any inter-node latency tests. Round-trip tests do not require PTP

The [gptp](gptp) directory contains scripts for time synchronization

Hardware timestamping is required on all nodes to be synced. The switch must support TSN standards and enable *gPTP* (IEEE 802.1AS) on all the ports. 

The Linux devices use `ptp4l` and `phc2sys` to sychronize network and system clocks respectively. Set the Grandmaster node (GM) and Follower nodes (FW) in the [gptp_env](gptp/gptp_env.sh). 

Execute `run_ptp.sh` to synchronize nodes and `kill_ptp.sh` to end synchronization. To simplify the process, you can run the following makefile target in the base directory to kill and restart sync:

```shell
make time-sync
```

It can take up to a minute for node system time to stabilize. Logs can be viewed using the `gptp_log.sh` script. Typically you should see a PHC sync under *10 us*.


### Benchmark Config File

A reference configuration file can be found [here](hc-mqtt.cfg)
- `broker`: Hostname of device running the MQTT broker
- `domain`: Domain extension provided by router DNS
- `mqtt_port`: Port on which MQTT broker is running
- `manifest`: File containing cluster device information. Refer to [devices.tsv](devices.tsv)
- `log_dir`: Directory to store result logs
- `out_dir`: Directory to store post-processed results


### Runnning benchmarks

Benchmark sets can be deployed using the `run_benchmark.py <base-args> <benchmark-type> <benchmark-args>` script. The `--help` option provides details on each parameter. The following `base-args` are supported
- `--config`: Configuration file to use (as constructed in previous section)
- `--interval`: Message publish interval
- `--iterations`: Number of data points to collect
- `--qos`: QoS of MQTT packets
- `--size`: Packet size
- `--drop-ratio`: Percentage of outliers to drop. Drop-ratio of *x*% only samples the middle *(100-x)*% of the packet transmission times
- `--log`: Enable logging. **NOTE**: Results cannot be post-processed with logs since it can corrupt the generated output. Use only for debugging functionality of the benchmark/deployment

All benchmark types can be found under [bench-scripts](bench-scripts). The following benchmarks are currently supported:
- `rtt_isolated`: Round-trip time for each node in isolation
- `rtt_nointerference`: Round-trip time for each node publishing simultaneously on different topics
- `rtt_fullinterference`: Round-trip time for each node publishing simultaneously on the same topic
- `one_to_one_isolated`: Inter-node single pub, single sub in isolation
- `fan_in --num <num-pub>`: Inter-node single sub, N publishers system on a single topic
- `fan_out --num <num-sub>`: Inter-node single pub, N subscribers system on a single topic


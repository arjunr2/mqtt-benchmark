# MQTT Network Benchmarking

Estimating pubsub network characteristics such as propagation and queuing delays are hard to model mathematically for distributed systems. This repository contains code to benchmark MQTT network characteristics (currently only latency) easily across custom communication topologies in a distributed cluster.

##  Setup
 
### PTP Time Synchronization

**NOTE**: PTP is only required if you are running any inter-node latency tests. Round-trip tests do not require PTP

The [gptp](gptp) directory contains scripts for time synchronization

Hardware timestamping is required on all nodes to be synced. The switch must support TSN standards and enable *gPTP* (IEEE 802.1AS) on all the ports. 

The Linux devices use `ptp4l` and `phc2sys` to sychronize network and system clocks respectively. Set the Grandmaster node (GM) and Follower nodes (FW) in the [gptp_env](gptp/gptp_env.sh). 

Execute `run_ptp.sh` to synchronize nodes and `kill_ptp.sh` to end synchronization. To simplify this process, a `time-sync` makefile target is provided in the base Makefile that can be run directly.

It can take up to a minute for node system time to stabilize. Logs can be viewed using the `gptp_log.sh` script. Typically you should see a PHC sync under *10 us*.


### Benchmark Config File

A reference configuration file can be found [here](hc-mqtt.cfg)
- `broker`: Hostname of device running the MQTT broker
- `domain`: Domain extension provided by router DNS
- `mqtt_port`: Port on which MQTT broker is running
- `manifest`: File containing cluster device information. Refer to [devices.tsv](devices.tsv)
- `log_dir`: Directory to store result logs
- `out_dir`: Directory to store post-processed results


### Benchmark Deployer

Benchmark sets can be deployed using the `run_benchmark.py <base-args> <benchmark> <benchmark-args>` script. The `--help` option provides details on each parameter. The following `base-args` are supported
- `--config (file)`: Configuration file to use (as constructed in previous section)
- `--interval (int)`: Message publish interval
- `--iterations (int)`: Number of data points to collect
- `--qos (int)`: QoS of MQTT packets [0, 1, 2]
- `--size (int)`: Packet size
- `--drop-ratio (int)`: Percentage of outliers to drop. Drop-ratio of *x*% only samples the middle *(100-x)*% of the packet transmission times
- `--log`: Enable logging. **NOTE**: Results cannot be post-processed with logs since it can corrupt the generated output. Use only for debugging functionality of the benchmark/deployment
- `--batch (file)`: File specifying sets of iterations, intervals, and sizes. Batch file overrides all the above arguments if specified and runs benchmark on Cartesian product of the sets. Refer to [batch.yml](batch.yml) as a sample

#### Supported Benchmarks

All benchmark types can be found under [bench_scripts](bench_scripts). The following benchmarks are currently supported:
- `rtt_isolated`: Round-trip time for each node in isolation
- `rtt_nointerference`: Round-trip time for each node publishing simultaneously on different topics
- `rtt_fullinterference`: Round-trip time for each node publishing simultaneously on the same topic
- `one_to_one_isolated`: Inter-node single pub, single sub in isolation
- `fan_in` `--num <num pubs>`: Inter-node single sub, N publishers system on a single topic
- `fan_out` `--num <num subs>`: Inter-node single pub, N subscribers system on a single topic



## Running Benchmarks

1. Restart time-synchronization, if you have not already: `make time-sync`
2. (Optional) Update and build pubsub code on cluster devices: `./update_bench,sh` 
3. Deploy benchmark set: `make start bench=<benchmark> args="<benchmark-args>" prerun=./update_bench.sh`
    - List of [supported benchmarks and arguments](#supported-benchmarks)
    - If running step 2, you can leave `prerun` empty. prerun` runs the provided command before benchmarking (useful for updating before run). 

Benchmarks are run in a separate screen when launched with `make start`. Real-time debug logging of the screen is dumped into `debug/<benchmark>.debug`. This can be viewed in real-time using `tail -f debug/<benchmark>.debug`.


## Writing New Custom Benchmarks

The existing benchmarks in [bench_scripts](bench_scripts) provide a good reference for writing benchmark configurations. A sample `fan_in` config is shown below, which runs all possible combinations on the cluster
```python
from .common import deploy, kill_pubs
from itertools import combinations

def _parse_sub(subparsers):
    parser = subparsers.add_parser("fan_in",
            help="Single subscriber, N publishers to single topic")
    parser.add_argument('-n', '--num', required=True,
            type=int, help = "Number of publishers")
    return parser

def _main(args, script_fmt):
    if args.num >= len(args.devices):
        raise ValueError(f"num ({args.num}) must be "\
                f"less than number of devices ({len(args.devices)})")
    sub_cmds = [
        "cd mqtt-benchmark",
        script_fmt.format(pub="", sub="topic")
    ]
    pub_cmds = [
        "cd mqtt-benchmark",
        script_fmt.format(pub="topic", sub="")
    ]

    outs = []
    for sub_dev in args.devices:
        pub_choices = [x for x in args.devices if x != sub_dev]
        for pub_devs in combinations(pub_choices, args.num):
            p = deploy (sub_cmds, [sub_dev], wait=False)
            deploy (pub_cmds, pub_devs, wait=False)
            stdout, stderr = p.communicate()
            kill_pubs (args.broker_addr, args.mqtt_port)
            outs.append(stdout)

    return '\n'.join(outs)

```

### Registering benchmark

- Each benchmark must implement a `_parse_sub` and a `_main` method. The former register the benchmark with the main subparser and any arguments the benchmark needs, while the latter runs the actual benchmark.
    - `_main` must return a string with all the relevant results to be logged for post-processing. In most cases, this is just the concatenation of the outputs from all the subscriber threads
- Add the new file to [bench_scripts/\_\_init\_\_.py](bench_scripts/__init__.py)
- Add the new file to list of SCRIPTS in [run_benchmark.py](run_benchmark.py)
 
 
**Important Notes**:
- `script_fmt` contains the deployment commands with static arguments populated. Each benchmark needs to only format the `pub` and `sub` field to indicate which topics to publish or subscribe to
- The `deploy` method sends the provided command to all provided devices. It can be made non-blocking by specifying `wait=False`
- **WARNING**: All publisher threads run in an infinite loop and subscriber threads run until `iterations` number of data points have been captured. The `kill_pubs` method is hence critical to kill all publishers after all subscribers have ended


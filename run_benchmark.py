import json
import yaml
import pandas as pd
from pathlib import Path
from itertools import product
import os

from argparse import ArgumentParser, Namespace, ArgumentDefaultsHelpFormatter
from bench_scripts import rtt_isolated, rtt_nointerference

SCRIPTS = [
    rtt_isolated,
    rtt_nointerference
]

def get_device_list(src):
    t = pd.read_csv(src, sep='\t')
    nodes = t[t['Type'] == 'linux']
    return list(nodes['Device'])

def _parse_main():
    p = ArgumentParser(
        description="MQTT benchmark deployment scripts",
        formatter_class=ArgumentDefaultsHelpFormatter)
    # Common Arguments
    p.add_argument("--config", 
            required = True,
            help = "Config file to load (.json)")
    p.add_argument("--interval", 
            nargs=1, default = [100000], type = int, 
            help = "Message interval period in us")
    p.add_argument("--iterations", 
            nargs=1, default = [100], type = int, 
            help = "Number of packets to send")
    p.add_argument("--qos", 
            nargs='?', default = 0, type = int, 
            help = "QoS of MQTT: [0-2])")
    p.add_argument("--size", 
            nargs=1, default = [64], type = int, 
            help = "Message size in bytes")
    p.add_argument("--drop-ratio", 
            nargs='?', default = 5, type = int, 
            help = "Percentage of outlier sample data to drop. "
            "Drops equally from min and max")
    p.add_argument("--log", action = "store_const", default="",
            const="--log",
            help = "Print log messages")
    p.add_argument("--batch", 
            nargs='?', default = None,
            help = "Batch param file to load (.yaml)")


    subparsers = p.add_subparsers(title="Benchmarks",
                    description="List of supported benchmarks",
                    dest="benchmark");
    subparsers.required = True
    # Add subparsers for each command
    for script in SCRIPTS:
        subparser = script._parse_sub(subparsers)
        # Call into scripts '_main' method
        subparser.set_defaults(_bench_main=script._main)
        
    return p


if __name__ == '__main__':
    p = _parse_main()
    args = p.parse_args()

    with open(args.config) as f:
        config_info = json.load(f)
        devices = get_device_list(config_info["manifest"])

    batch_info = {}
    arg_dict = vars(args)
    if args.batch is not None:
        with open(args.batch, "r") as f:
            batch_info = yaml.safe_load(f)
            # Batch info overwrites args
            arg_dict.update(batch_info)

    args = Namespace(**arg_dict, **config_info, devices=devices)
    
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(args.out_dir, exist_ok=True)

    # Bench Main only needs to change pub, sub formats topics 
    address = f"{args.broker}{args.domain}:{args.mqtt_port}"
    name = "\`hostname\`"
    for iterations, interval, size in product(args.iterations, args.interval, args.size):
        script_fmt = f"./benchmark --broker={address} --name={name} --interval={interval} "\
            f"--iterations={iterations} --size={size} --pub={{pub}} --sub={{sub}} "\
            f"--qos={args.qos} --drop-ratio={args.drop_ratio} {args.log}"

        # Get deployment log
        log_out = args._bench_main(args, script_fmt)

        fbasename = f"{args.benchmark}_m{interval}_s{size}"
        logfile = Path(args.log_dir) / f"{fbasename}.log"
        outfile = Path(args.out_dir) / f"{fbasename}.out"
        with open(logfile, "w") as f:
            f.write(log_out)
    
    


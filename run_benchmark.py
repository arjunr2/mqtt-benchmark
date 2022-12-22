import json
import pandas as pd

from argparse import ArgumentParser, Namespace, ArgumentDefaultsHelpFormatter
from bench_scripts import local_isolated, local_nointerference

SCRIPTS = [
    local_isolated,
    local_nointerference
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
            nargs='?', default = 100000, type = int, 
            help = "Message interval period in us")
    p.add_argument("--iterations", 
            nargs='?', default = 100, type = int, 
            help = "Number of packets to send")
    p.add_argument("--qos", 
            nargs='?', default = 0, type = int, 
            help = "QoS of MQTT: [0-2])")
    p.add_argument("--size", 
            nargs='?', default = 64, type = int, 
            help = "Message size in bytes")
    p.add_argument("--drop-ratio", 
            nargs='?', default = 5, type = int, 
            help = "Percentage of outlier sample data to drop. "
            "Drops equally from min and max")
    p.add_argument("--log", action = "store_const", default="",
            const="--log",
            help = "Print log messages")
    p.add_argument("--outfile", 
            nargs='?', default = None,  
            help = "Output file path")


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

    if args.outfile is None:
        args.outfile = args.benchmark + ".results"

    with open(args.config) as f:
        config_info = json.load(f)
        devices = get_device_list(config_info["manifest"])

    # Merge config info
    args = Namespace(**vars(args), **config_info, devices=devices)
    # Bench Main only needs to change pub, sub formats topics 
    address = f"{args.broker}{args.domain}:{args.mqtt_port}"
    name = "\`uuidgen\`"
    script_fmt = f"./benchmark --broker={address} --name={name} --interval={args.interval} "\
        f"--iterations={args.iterations} --size={args.size} --pub={{pub}} --sub={{sub}} "\
        f"--qos={args.qos} --drop-ratio={args.drop_ratio} {args.log}"

    args._bench_main(args, script_fmt)
    
    


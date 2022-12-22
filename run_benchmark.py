import json
import pandas as pd

from argparse import ArgumentParser, Namespace
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
        description="MQTT benchmark deployment scripts")
    p.add_argument("--config", required=True,
        help="Config file to load (.json)")

    subparsers = p.add_subparsers(title="Benchmarks",
                    description="List of supported benchmarks");
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
    # Merge config info
    args = Namespace(**vars(args), **config_info)
    print(args)    
    args._bench_main(args)
    
    


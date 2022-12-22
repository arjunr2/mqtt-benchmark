from argparse import ArgumentParser, RawTextHelpFormatter
from bench_scripts import local_isolated, local_nointerference

SCRIPTS = [
    local_isolated,
    local_nointerference
]

def _help_list():
    lines = ["{:<20}  {}".format(x, SCRIPTS[x][-1]) for x in SCRIPTS]
    return '\n'.join(lines)

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
        subparser.set_defaults(_bench_main=script._main)
        
    return p


if __name__ == '__main__':
    p = _parse_main()
    args = p.parse_args()

    args._bench_main(args)

    


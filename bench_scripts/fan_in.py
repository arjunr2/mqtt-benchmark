from .common import deploy, kill_bench
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
            kill_bench (pub_devs)
            outs.append(stdout)

    return '\n'.join(outs)

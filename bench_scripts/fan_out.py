from .common import deploy, kill_pubs
from itertools import combinations

def _parse_sub(subparsers):
    parser = subparsers.add_parser("fan_out",
            help="N subscribers, single publisher to single topic")
    parser.add_argument('-n', '--num', required=True,
            type=int, help = "Number of subscribers")
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
    for pub_dev in args.devices:
        sub_choices = [x for x in args.devices if x != pub_dev]
        for sub_devs in combinations(sub_choices, args.num):
            p = deploy (sub_cmds, sub_devs, wait=False)
            deploy (pub_cmds, [pub_dev], wait=False)
            stdout, stderr = p.communicate()
            kill_pubs (args.broker_addr, args.mqtt_port)
            outs.append(stdout)

    return '\n'.join(outs)

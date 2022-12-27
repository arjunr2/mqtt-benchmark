from .common import deploy, kill_pubs
from itertools import combinations

def _parse_sub(subparsers):
    parser = subparsers.add_parser("one_to_one_isolated",
            help="Single-pub, single-sub link across nodes in isolation")
    return parser

def _main(args, script_fmt):
    sub_cmds = [
        "cd mqtt-benchmark",
        script_fmt.format(pub="", sub="topic")
    ]
    pub_cmds = [
        "cd mqtt-benchmark",
        script_fmt.format(pub="topic", sub="")
    ]

    outs = []
    combs = list(combinations(args.devices, 2))
    all_combs = combs + [t[::-1] for t in combs]
    for d1, d2 in all_combs:
        p = deploy (sub_cmds, [d1], wait=False)
        deploy (pub_cmds, [d2], wait=False)
        stdout, stderr = p.communicate()
        kill_pubs (args.broker_addr, args.mqtt_port)
        outs.append(stdout)

    return '\n'.join(outs)

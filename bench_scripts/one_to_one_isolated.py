from .common import deploy
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
        script_fmt.format(pub"topic", sub="")
    ]

    outs = []
    combs = combinations(args.devices, 2)
    for d1, d2 in combs:
        p = deploy (sub_cmds, [d1], wait=False)
        deploy (pub_cmds, [d2], wait=False)
        stdout, stderr = p.communicate()
        outs.append(stdout)

        p = deploy (sub_cmds, [d2], wait=False)
        deploy (pub_cmds, [d1], wait=False)
        stdout, stderr = p.communicate()
        outs.append(stdout)

    return '\n'.join(outs)

from .common import deploy

def _parse_sub(subparsers):
    parser = subparsers.add_parser("one_to_one_isolated",
            help="Single-pub, single-sub link across nodes in isolation")
    return parser

def _main(args, script_fmt):
    sub_cmd = [
        "cd mqtt-benchmark",
        script_fmt.format(pub="", sub="topic")
    ]
    pub_cmd = [
    
    ]

    return deploy (cmd_list, args.devices)

from .common import deploy

def _parse_sub(subparsers):
    parser = subparsers.add_parser("rtt_fullinterference",
            help="Round-trip time simultaneously on all nodes interfering")
    return parser

def _main(args, script_fmt):
    fields = {}
    fields["pub"] = "topic"
    fields["sub"] = fields["pub"]
    cmd_list = [
        "cd mqtt-benchmark",
        script_fmt.format(**fields)
    ]

    return deploy (cmd_list, args.devices)

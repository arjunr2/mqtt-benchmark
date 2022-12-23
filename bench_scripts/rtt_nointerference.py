from .common import deploy

def _parse_sub(subparsers):
    parser = subparsers.add_parser("rtt_nointerference",
            help="Round-trip time simultaneously on non-interfering topics")
    return parser

def _main(args, script_fmt):
    fields = {}
    fields["pub"] = "\$TOPIC"
    fields["sub"] = fields["pub"]
    cmd_list = [
        "cd mqtt-benchmark",
        "TOPIC=\`uuidgen\`",
        script_fmt.format(**fields)
    ]

    return deploy (cmd_list, args.devices)


def _parse_sub(subparsers):
    parser = subparsers.add_parser("local_isolated",
            help="RTT same node simultaneously on non-interfering topics")
    return parser

def _main(args, script_fmt):
    fields = {}
    fields["pub"] = "topic"
    fields["sub"] = fields["pub"]
    script_str = "cd mqtt-benchmark; " + script_fmt.format(**fields)


    deploy_cmd = "hc cmd --sync -x \"{}\"". format(script_str)
    print(deploy_cmd)

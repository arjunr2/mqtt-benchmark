from .common import construct_deploy

def _parse_sub(subparsers):
    parser = subparsers.add_parser("local_isolated",
            help="RTT same node simultaneously on non-interfering topics")
    return parser

def _main(args, script_fmt):
    fields = {}
    fields["pub"] = "topic"
    fields["sub"] = fields["pub"]
    cmd_list = [
        "cd mqtt-benchmark",
        script_fmt.format(**fields)
    ]
    
    deploy_cmd = construct_deploy (cmd_list, args.devices, sync=True)
    print(deploy_cmd)

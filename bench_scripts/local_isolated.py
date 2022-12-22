
def _parse_sub(subparsers):
    parser = subparsers.add_parser("local_isolated",
            help="RTT same node simultaneously on non-interfering topics")
    parser.add_argument('value', help="Some value for no isolated")
    return parser

def _main(args):
    print(args)

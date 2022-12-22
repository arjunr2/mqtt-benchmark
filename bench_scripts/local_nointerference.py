
def _parse_sub(subparsers):
    parser = subparsers.add_parser("local_nointerference",
            help="RTT same node simultaneously on non-interfering topics")
    parser.add_argument('vnif', help="Some value for no interference")
    return parser

def _main(args):
    print(args)

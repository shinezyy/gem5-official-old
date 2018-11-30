def addSSOptions(parser):
    parser.add_option("--arch",
            choices=['ARM', 'RISCV'],
            help="ARM or RISCV must be specified")


def addO3Options(parser):
    parser.add_option("--num-ROB",
            default=9,
            action='store', type='int',
            help="num ROB entries")

    parser.add_option("--num-IQ",
            default=9,
            action='store', type='int',
            help="num IQ entries")

    parser.add_option("--num-LQ",
            default=9,
            action='store', type='int',
            help="num LQ entries")

    parser.add_option("--num-SQ",
            default=9,
            action='store', type='int',
            help="num SQ entries")

    parser.add_option("--num-PhysReg",
            default=9,
            action='store', type='int',
            help="num physical registers")


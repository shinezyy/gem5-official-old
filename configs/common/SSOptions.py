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

    parser.add_option("--bp-size",
            action='store', type=int,
            help="Global predictor size")

    parser.add_option("--bp-index-type",
            action='store', type=str,
            help="Indexing method of perceptronBP")

    parser.add_option("--bp-history-len",
            action='store', type=int,
            help="History length(size of each perceptron")

    parser.add_option("--bp-learning-rate",
            action='store', type=int,
            help="Learning rate of perceptronBP")

    parser.add_option("--bp-pseudo-tagging",
            action='store', type=int,
            help="Num bits of pseudo-tagging")

    parser.add_option("--bp-dyn-thres",
            action='store', type=int,
            help="log2 of num theta used")

    parser.add_option("--bp-tc-bit",
            action='store', type=int,
            help="Threshold counter bit, valid when dyn-thres > 0")

    parser.add_option("--bp-weight-bit",
            action='store', type=int,
            help='Bits used to store weights')

    parser.add_option("--bp-redundant-bit",
            action='store', type=int,
            help='n-bits to represent a history bit')

    parser.add_option("--use-ltage",
            action='store_true',
            help='Use L-TAGE as branch predictor')

    parser.add_option("--use-tournament",
            action='store_true',
            help='Use Tournament as branch predictor')

    parser.add_option("--use-ogb",
            action='store_true',
            help='Use OGB as branch predictor')

    parser.add_option("--use-nbbp",
            action='store_true',
            help='Use naive bayes as branch predictor')

    parser.add_option("--use-zperceptron",
            action='store_true',
            help='Use zperceptron as branch predictor')

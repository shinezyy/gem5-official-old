def addSSOptions(parser):

    # system options
    parser.add_option("--arch",
            choices=['ARM', 'RISCV'],
            help="ARM or RISCV must be specified")

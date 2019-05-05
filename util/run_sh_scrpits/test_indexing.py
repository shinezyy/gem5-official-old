#!/usr/bin/env python3

import os
import subprocess
from test import run
import math as m

rv_origin = './rv-origin.py'
options = [rv_origin]

size   = [128, 256, 512]
hislen = [59, 36]
indexing = ['MODULO',\
            'BITWISE_XOR',\
            'PRIME_MODULO',\
            'PRIME_DISPLACEMENT']

num_threads = 6

for i in range(len(size)):
    options += ['--num-threads={}'.format(num_threads),
                '--bp-size={}'.format(size[i]),
                '--bp-history-len={}'.format(hislen[i]),
                '--bp-pseudo-tagging={}'.format(p_bits[i]),
                '-a']
    run(options)
    options = [rv_origin]

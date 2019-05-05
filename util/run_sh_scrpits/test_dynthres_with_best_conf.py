#!/usr/bin/env python3

import sh
import subprocess
from test import run

size = 546
hislen = 59

dyn_bit = [10]
tc_bit  = [4, 5, 6, 7, 8]

num_thread = 6

rv_origin = './rv-origin.py'
options = [rv_origin]

dyn_comb = [[d, t] for d in dyn_bit for t in tc_bit]

for [d, t] in dyn_comb:
    options += ['--num-threads={}'.format(num_thread),
                '--bp-size={}'.format(size),
                '--bp-history-len={}'.format(hislen),
                '--bp-dyn-thres={}'.format(d),
                '--bp-tc-bit={}'.format(t),
                '-a']
    run(options)
    options = [rv_origin]

#!/usr/bin/env python3

import os
import subprocess
from test import run
import math as m

rv_origin = './rv-origin.py'
options = [rv_origin]

size   = [546, 442]
hislen = [59, 36]

p_bits = [int(h/8) for h in hislen]
print(p_bits)

num_threads = 12

for i in range(len(size)):
    options += ['--num-threads={}'.format(num_threads),
                '--bp-size={}'.format(size[i]),
                '--bp-history-len={}'.format(hislen[i]),
                '--bp-pseudo-tagging={}'.format(p_bits[i]),
                '-a']
    run(options)
    options = [rv_origin]

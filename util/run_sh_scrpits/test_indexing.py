#!/usr/bin/env python3

import os
import subprocess
from test import run, index

rv_origin = './rv-origin.py'

options = [rv_origin]

size = [128, 256, 512]

comb = [[s, i] for s in size for i in index]

num_threads = 6

h = 16

for [s, i] in comb:
    options += ['--num-threads={}'.format(num_threads),
                '--bp-size={}'.format(s),
                '--bp-index-type={}'.format(i),
                '--bp-history-len={}'.format(h),
                '-a']
    run(options)
    options = [rv_origin]

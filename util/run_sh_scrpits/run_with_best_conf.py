#!/usr/bin/env python3

import sh
import subprocess

size   = [161, 267, 506, 624, 1248]
hislen = [28, 34, 36, 59, 59]
w_bit  = [7, 7, 7, 7, 7]

def run(command):
    out = subprocess.check_output(command)
    out_text = out.decode('utf-8')
    print(out_text)

num_thread = 12

rv_origin = './rv-origin.py'
options = [rv_origin]

for i in range(5):
    options += ['--num-threads={}'.format(num_thread),
                '--bp-size={}'.format(size[i]),
                '--bp-history-len={}'.format(hislen[i]),
                '--bp-weight-bit={}'.format(w_bit[i]),
                '-a']
    run(options)
    options = [rv_origin]


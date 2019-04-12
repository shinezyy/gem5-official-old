#!/usr/bin/env python3

import os
import sys
import sh
import time
from os.path import join as pjoin
from os.path import expanduser as uexp
from multiprocessing import Pool
from multiprocessing import cpu_count
import argparse
import subprocess

num_bench = 22


size       = [2048, 4096, 8192]
index      = ['MODULO',\
              'BITWISE_XOR',\
              'PRIME_DISPLACEMENT',\
              'PRIME_MODULO']
hislen     = [32, 64]
lr         = [1]
pseudo_tag = [6, 8]
dyn_thres  = [6, 8]
tc_bit     = [4, 5, 6, 7, 8]
w_bit      = [6, 7, 8]

params = [size, index, hislen, lr, pseudo_tag, dyn_thres, tc_bit, w_bit]

def parser_add_arguments(parser):

    group = parser.add_mutually_exclusive_group()

    group.add_argument('-d', '--default', action='store_true',
                        help='Run with default params')

    group.add_argument('-a', '--all', action='store_true',
                        help='Run with all possible combination of params')

    combs = parser.add_mutually_exclusive_group()

    combs.add_argument('-i', '--index', action='store_true',
                       help='Run with all index methods')

    combs.add_argument('-b', '--budget', action='store_true',
                       help='Run with all sizes and hislens')

    combs.add_argument('-p', '--pseudo-tagging', action='store_true',
                       help='Running with all possible pseudo-tagging bits')

    combs.add_argument('--dynamic-thres', action='store_true',
                       help='Running with all dynamic threshold configs')

    parser.add_argument('--all-benchmarks', action='store_true',
                        help='Switch to run all 22 benchmarks')

def run(command):
    out = subprocess.check_output(command)
    out_text = out.decode('utf-8')
    print(out_text)

def main():
    usage_str = '[-d | -a | -i | -b | -p | --dynamic-thres]'
    parser = argparse.ArgumentParser(usage=usage_str)

    parser_add_arguments(parser)

    opt = parser.parse_args()

    if cpu_count() / 2 >= num_bench:
        num_thread = 22
    else:
        num_thread = int(cpu_count() / 2)
    print('num thread is', num_thread)

    rv_origin = './rv-origin.py'

    options = [rv_origin]

    if opt.default:
        run(options)
    else:
        options = [rv_origin]
        if opt.all:
            combinations = [[s, i, h, l, p, d, t, w] for s in size
                    for i in index for h in hislen for l in lr
                    for p in pseudo_tag for d in dyn_thres
                    for t in tc_bit for w in w_bit]
            for [s, i, h, l, p, d, t, w] in combinations:
                options += ['--num-threads={}'.format(num_thread),
                            '--bp-size={}'.format(s),
                            '--bp-index-type={}'.format(i),
                            '--bp-history-len={}'.format(h),
                            '--bp-learning-rate={}'.format(l),
                            '--bp-pseudo-tagging={}'.format(p),
                            '--bp-dyn-thres={}'.format(d),
                            '--bp-tc-bit={}'.format(t),
                            '--bp-weight-bit={}'.format(w)]
                if opt.all_benchmarks:
                    options += ['-a']
                run(options)
        else:
            if opt.index:
                for i in index:
                    options += ['--num-threads={}'.format(num_thread),
                                '--bp-index-type={}'.format(i)]
                    if opt.all_benchmarks:
                        options += ['-a']
                    run(options)
            if opt.budget:
                combinations = [[s, h] for s in size for h in hislen]
                for [s, h] in combinations:
                    options += ['--num-threads={}'.format(num_thread),
                                '--bp-size={}'.format(s),
                                '--bp-history-len={}'.format(h)]
                    if opt.all_benchmarks:
                        options += ['-a']
                    run(options)
            if opt.pseudo_tagging:
                for p in pseudo_tag:
                    options += ['--num-threads={}'.format(num_thread),
                                '--bp-pseudo-tagging={}'.format(p)]
                    if opt.all_benchmarks:
                        options += ['-a']
                    run(options)
            if opt.dynamic_thres:
                combinations = [[d, t] for d in dyn_thres for t in tc_bit]
                for [d, t] in combinations:
                    options += ['--num-threads={}'.format(num_thread),
                                '--bp-dyn-thres={}'.format(d),
                                '--bp-tc-bit={}'.format(t)]
                    if opt.all_benchmarks:
                        options += ['-a']
                    run(options)



if __name__ == '__main__':
    main()


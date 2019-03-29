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

num_bench = 22


size       = [2048, 4096, 8192]
index      = ['MODULO',\
              'BITWISE_XOR',\
              'PRIME_DISPLACEMENT',\
              'PRIME_MODULO']
hislen     = [32, 64]
lr         = [1]
pseudo_tag = [0, 6, 8]
dyn_thres  = [0, 6, 8]
tc_bit     = [0, 6, 7, 8]

params = [size, index, hislen, lr, pseudo_tag, dyn_thres, tc_bit]

def parser_add_arguments(parser):

    group = parser.add_mutually_exclusive_group()

    group.add_argument('-d', '--default', action='store_true',
                        help='Run with default params')

    group.add_argument('-a', '--all', action='store_true',
                        help='Run with all possible combination of params')

    parser.add_argument('-i', '--index', action='store_true',
                        help='Run with all index methods')

    parser.add_argument('--all-benchmarks', action='store_true',
                        help='Switch to run all 22 benchmarks')

def main():
    parser = argparse.ArgumentParser(usage='[-d | -a]')

    parser_add_arguments(parser)

    opt = parser.parse_args()

    if cpu_count() / 2 >= num_bench:
        num_thread = 22
    else:
        num_thread = int(cpu_count() / 2)
    print('num thread is', num_thread)

    rv_origin = sh.Command('./rv-origin.py')

    options = []

    if opt.default:
        rv_origin()
    else:
        options = []
        if opt.all:
            combinations = [[s, i, h, l, p, d, t] for s in size for i in index\
                    for h in hislen for l in lr for p in pseudo_tag\
                    for d in dyn_thres for t in tc_bit]
            for [s, i, h, l, p, d, t] in combinations:
                options = ['--num-threads={}'.format(num_thread),
                           '--bp-size={}'.format(s),
                           '--bp-index-type={}'.format(i),
                           '--bp-history-len={}'.format(h),
                           '--bp-learning-rate={}'.format(l),
                           '--bp-pseudo-tagging={}'.format(p),
                           '--bp-dyn-thres={}'.format(d),
                           '--bp-tc-bit={}'.format(t)]
                if opt.all_benchmarks:
                    options += ['-a']
                rv_origin(*options)
        elif opt.index:
            for i in index:
                options = ['--num-threads={}'.format(num_thread),
                           '--bp-index-type={}'.format(i)]
                if opt.all_benchmarks:
                    options += ['-a']
                rv_origin(*options)

if __name__ == '__main__':
    main()


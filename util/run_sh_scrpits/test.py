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

size       = [1024, 2048, 4096, 8192]
index      = ['MODULO',\
              'BITWISE_XOR',\
              'PRIME_DISPLACEMENT',\
              'PRIME_MODULO']
hislen     = [32, 64]
lr         = [1, 2, 3]
pseudo-tag = [0, 4, 6, 8]
dyn-thres  = [0, 5, 6, 7, 8]
tc-bit     = [0, 5, 6, 7, 8]

def parser_add_arguments(parser):

    group = parser.add_mutually_exclusive_group()

    group.add_argument('-d', '--default', action='store_true',
                        default=True,
                        help='Run with default params')

    group.add_argument('-a', '--all', action='store_true',
                        default=False,
                        help='Run with all possible combination of params')

def main():
    parser = argparse.ArgumentParser(usage='[-d | -a]')

    parser_add_argument(parser)

    opt = parser.parse_args()

    if cpu_count / 2 >= num_bench:
        num_thread = 22
    else:
        num_thread = int(cpu_count / 2)

    rv-origin = sh.Command('python3 rv-origin.py')

    if opt.default:
        rv-origin()
    else:
        if opt.all:
            for s, i, h, l, p, d, t in \
                zip(size, index, hislen, lr, pseudo-tag, dyn-thres, tc-bit):
                options = ['--bp-size='+s,
                           '--bp-index-type='+i,
                           '--bp-history-len='+h,
                           '--bp-learning-rate='+l,
                           '--bp-pseudo-tagging='+p,
                           '--bp-dyn-thres='+d,
                           '--bp-tc-bit='+t]
                rv-origin(*options)


if __name__ == '__main__':
    main()


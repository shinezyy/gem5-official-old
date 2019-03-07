#!/usr/bin/env python3

import os
import re
import sys
import random
import sh
import time
from os.path import join as pjoin
from os.path import expanduser as uexp
from multiprocessing import Pool
import common as c


outdir = uexp('~/gem5-results/micro-benchmarks/')

arch = 'RISCV'

def example_to_run_other_binary(cmd, some_extra_args, outdir_b):

    interval = 200*10**6
    warmup = 20*10**6

    os.chdir(c.gem5_exec())

    options = [
            '--outdir=' + outdir_b,
            '--debug-flag=PErceptron',
            pjoin(c.gem5_home(), 'configs/ss/se.py'),
            '-c',
            '{}'.format(cmd),
            '-I {}'.format(220*10**6),
            '--mem-size=4GB',
            '--arch={}'.format(arch),
            ]
    cpu_model = 'OoO'
    if cpu_model == 'TimingSimple':
        options += [
                '--cpu-type=TimingSimpleCPU',
                '--mem-type=SimpleMemory',
                ]
    elif cpu_model == 'OoO':
        options += [
            #'--debug-flag=Fetch',
            '--cpu-type=DerivO3CPU',
            '--mem-type=DDR3_1600_8x8',

            '--caches',
            '--cacheline_size=64',

            '--l1i_size=32kB',
            '--l1d_size=32kB',
            '--l1i_assoc=8',
            '--l1d_assoc=8',

            '--l2cache',
            '--l2_size=4MB',
            '--l2_assoc=8',

            '--num-ROB=300',
            '--num-IQ=128',
            '--num-LQ=100',
            '--num-SQ=100',
            ]
    else:
        assert False
    print(options)
    gem5 = sh.Command(pjoin(c.gem5_build(arch), 'gem5.opt'))
    # sys.exit(0)
    gem5(
            _out=pjoin(outdir_b, 'gem5_out.txt'),
            _err=pjoin(outdir_b, 'gem5_err.txt'),
            *options
            )


def run(args):
    cmd, sub_dir = args
    outdir_b = pjoin(outdir, sub_dir)
    if not os.path.isdir(outdir_b):
        os.makedirs(outdir_b)

    prerequisite = True
    some_extra_args = None

    if prerequisite:
        print('prerequisite satisified, is going to run {} on gem5'.format(cmd))
        c.avoid_repeated(example_to_run_other_binary, outdir_b,
                cmd, some_extra_args, outdir_b)
    else:
        print('prerequisite not satisified, abort on', cmd)


def main():
    num_thread = 1

    cmds = [(pjoin(c.gem5_home(), 'tests/ss/large-fanout-rand-array'),
        'rand-array')]

    if num_thread > 1:
        p = Pool(num_thread)
        p.map(run, cmds)
    else:
        run(cmds[0])


if __name__ == '__main__':
    main()

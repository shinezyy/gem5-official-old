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


outdir = '/ramdisk/zyy/gem5_run/results/test'


def example_to_run_batch(benchmark, some_extra_args, outdir_b):

    gem5_dir = c.gem5_home()

    interval = 200*10**6
    warmup = 20*10**6

    exec_dir = c.gem5_exec()
    os.chdir(exec_dir)

    options = [
            '--outdir=' + outdir_b,
            pjoin(gem5_dir, 'configs/spec2006/se_spec06.py'),
            '--spec-2006-bench',
            '-b',
            '{}'.format(benchmark),
            '--benchmark-stdout={}/out'.format(outdir_b),
            '--benchmark-stderr={}/err'.format(outdir_b),
            '--cpu-type=AtomicSimpleCPU',
            '-I {}'.format(1*10**6),
            '--fastmem',
            '--mem-size=4GB',
            ]
    print(options)
    gem5 = sh.Command(pjoin(c.gem5_build(), 'gem5.opt'))
    # sys.exit(0)
    gem5(
            _out=pjoin(outdir_b, 'gem5_out.txt'),
            _err=pjoin(outdir_b, 'gem5_err.txt'),
            *options
            )


def run(benchmark):
    outdir_b = pjoin(outdir, benchmark)
    if not os.path.isdir(outdir_b):
        os.makedirs(outdir_b)

    prerequisite = True
    some_extra_args = None

    if prerequisite:
        print('prerequisite satisified, is going to run gem5 on', benchmark)
        c.avoid_repeated(example_to_run_batch, outdir_b,
                benchmark, some_extra_args, outdir_b)
    else:
        print('prerequisite not satisified, abort on', benchmark)


def main():
    num_thread = 2

    benchmarks = []

    with open('./tmp_spec.txt') as f:
        for line in f:
            benchmarks.append(line.strip())

    if num_thread > 1:
        p = Pool(num_thread)
        p.map(run, benchmarks)
    else:
        run(benchmarks[0])


if __name__ == '__main__':
    main()

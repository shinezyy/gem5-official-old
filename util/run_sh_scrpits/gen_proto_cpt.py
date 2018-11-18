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


outdir = '/ramdisk/zyy/gem5_run/deptrace'

arch = 'RISCV'

def example_to_restore_cpt(benchmark, some_extra_args, outdir_b):

    os.chdir(c.gem5_exec())

    options = [
            # '--debug-flags=ElasticTrace',
            # '--debug-start=35490356680500',
            # '--debug-end=35490397513000',
            '--outdir=' + outdir_b,
            pjoin(c.gem5_home(), 'configs/spec2006/se_spec06.py'),
            '--spec-2006-bench',
            '-b',
            '{}'.format(benchmark),
            '--benchmark-stdout={}/out'.format(outdir_b),
            '--benchmark-stderr={}/err'.format(outdir_b),
            '-I {}'.format(200*10**6),
            '--mem-size=4GB',
            '-r 1',
            '--elastic-trace-en',
            '--data-trace-file=deptrace.proto.gz',
            '--inst-trace-file=fetchtrace.proto.gz',
            '--mem-type=SimpleMemory',
            '--restore-simpoint-checkpoint',
            '--restore-with-cpu=DerivO3CPU',
            '--checkpoint-dir={}'.format(pjoin(c.gem5_cpt_dir(arch),
                benchmark)),
            '--arch={}'.format(arch)
            ]
    cpu_model = 'OoO'
    if cpu_model == 'TimingSimple':
        options += [
                '--cpu-type=TimingSimpleCPU',
                '--mem-type=SimpleMemory',
                ]
    elif cpu_model == 'OoO':
        options += [
            '--cpu-type=DerivO3CPU',

            '--caches',
            '--cacheline_size=64',

            '--l1i_size=32kB',
            '--l1d_size=32kB',
            '--l1i_assoc=8',
            '--l1d_assoc=8',
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


def run(benchmark):
    outdir_b = pjoin(outdir, benchmark)
    if not os.path.isdir(outdir_b):
        os.makedirs(outdir_b)

    prerequisite = True
    some_extra_args = None

    if prerequisite:
        print('prerequisite satisified, is going to run gem5 on', benchmark)
        c.avoid_repeated(example_to_restore_cpt, outdir_b,
                benchmark, some_extra_args, outdir_b)
    else:
        print('prerequisite not satisified, abort on', benchmark)


def main():
    num_thread = 4

    benchmarks = []

    with open('./all_function_spec.txt') as f:
        for line in f:
            benchmarks.append(line.strip())

    if num_thread > 1:
        p = Pool(num_thread)
        p.map(run, benchmarks)
    else:
        run(benchmarks[0])


if __name__ == '__main__':
    main()

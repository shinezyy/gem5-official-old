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


def run(benchmark):
    global opt

    gem5_dir = c.gem5_home()
    outdir = pjoin(uexp('~/alpha_trace'), benchmark)
    cpt_dir = pjoin('/home/share/st_checkpoint', benchmark)

    if not os.path.isdir(outdir):
        os.makedirs(outdir)

    exec_dir = c.gem5_exec()
    os.chdir(exec_dir)

    options = [
            '--outdir=' + outdir,
            pjoin(gem5_dir, 'configs/spec2006/se_spec06.py'),
            '--mem-size=8GB',
            '--spec-2006-bench',
            '-b',
            '{}'.format(benchmark),
            '--benchmark-stdout={}/out'.format(outdir),
            '--benchmark-stderr={}/err'.format(outdir),
            '--cpu-type=DerivO3CPU',
            '--caches',
            '--cacheline_size=64',
            '--l1i_size=32kB',
            '--l1d_size=32kB',
            '--l1i_assoc=1',
            '--l1d_assoc=1',
            '-I 200000000',
            '--elastic-trace-en',
            '--data-trace-file=deptrace.proto.gz',
            '--inst-trace-file=fetchtrace.proto.gz',
            '--mem-type=SimpleMemory',
            '--restore-simpoint-checkpoint',
            '--restore-with-cpu=DerivO3CPU',
            '--checkpoint-dir={}'.format(cpt_dir),
            ]
    print options
    # sys.exit(0)
    sh.gem5_opt(
            _out=pjoin(outdir, 'gem5_out.txt'),
            _err=pjoin(outdir, 'gem5_err.txt'),
            *options
            )

    sh.touch(pjoin(outdir, 'done'))

def main():
    num_thread = 3

    benchmarks = []
    with open('./all_function_spec.txt') as f:
        for line in f:
            benchmarks.append(line.strip())
    # print benchmarks

    if num_thread > 1:
        p = Pool(num_thread)
        p.map(run, benchmarks)
    else:
        run(benchmarks[0])


if __name__ == '__main__':
    main()

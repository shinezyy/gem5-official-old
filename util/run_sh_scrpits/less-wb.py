#!/usr/bin/env python2.7

import os
import re
import sys
import random
import sh
import time
from os.path import join as pjoin
from os.path import expanduser as uexp
from multiprocessing import Pool

cmd_timestamp = None


def run(benchmark):
    global cmd_timestamp

    gem5_dir = os.environ['gem5_root']
    outdir = pjoin(uexp('~/gem5-results/normal-outputs'), benchmark)
    cpt_dir = pjoin('/home/share/st_checkpoint', benchmark)

    if not os.path.isdir(outdir):
        os.makedirs(outdir)

    if cmd_timestamp:
        output_timestamp_file = pjoin(outdir, 'done')
        if os.path.isfile(output_timestamp_file):
            file_m_time = os.path.getmtime(output_timestamp_file)
            if file_m_time > cmd_timestamp:
                print('Command is older than output of {}, skip!'.format(
                    benchmark))
                return

    exec_dir = os.environ['gem5_run_dir']
    os.chdir(exec_dir)

    options = [
            '--outdir=' + outdir,
            pjoin(gem5_dir, 'configs/spec2006/se_spec06.py'),
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
            '--l2cache',
            '--l2_size=4MB',
            '--l2_assoc=8',
            '-I 200000000',
            '--mem-size=4GB',
            '--restore-simpoint-checkpoint',
            '--restore-with-cpu=DerivO3CPU',
            '--checkpoint-dir={}'.format(cpt_dir),
            ]
    print(options)
    # sys.exit(0)
    sh.gem5_opt(
            _out=pjoin(outdir, 'gem5_out.txt'),
            _err=pjoin(outdir, 'gem5_err.txt'),
            *options
            )

    sh.touch(pjoin(outdir, 'done'))

def main():
    num_thread = 4

    benchmarks = []

    cmd_timestamp_file = './less-wb-cmd-timestamp'
    global cmd_timestamp
    if os.path.isfile(cmd_timestamp_file):
        cmd_timestamp = os.path.getmtime(cmd_timestamp_file)
        print(cmd_timestamp)


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

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
import argparse

numIQ = 128

debug_flag = 'MYperceptron'

target_function = 'all_function_spec.txt'

bp_types = ['LTAGE',\
            'TournamentBP',\
            'PerceptronLocal',\
            'Perceptron',\
            'PathPerceptron',\
            'LocalBP',\
            'MyPerceptron']

bp_params = ['',\
             '_size_16',\
             '_size_20',\
             '_size_32',\
             '_size_64',\
             '_size_128']

indexing = ['_modulo',\
            '_bitwiseXor',\
            '_iPoly',\
            '_primeModulo',\
            '_primeDisplacement']

lrs = ['_lamda1', '_lamda2', '_lamda3']

alternative = ['',\
               '_alt',\
               '_debug',\
               '_aliasing',\
               '_pseudo-tagging',\
               '_theta',\
               '_dynamic-threshold']

dyn_size = ['', '_dyn4', '_dyn7']

bp_type = bp_types[6]

bp_param = bp_params[4]

index = indexing[0]

lr = lrs[0]

alt = alternative[3]# + alternative[6]

dyn = dyn_size[0]

outdir = \
    '/home/glr/gem5/gem5-results/test_'\
        + bp_type + bp_param + index + lr + alt + dyn

arch = 'RISCV'

def parser_add_arguments(parser):
    parser.add_argument('-n', '--num-threads', action='store', type=int,
                        help='Num threads used to run benchmarks')

    group = parser.add_mutually_exclusive_group()

    group.add_argument('-s', '--specified-benchmark', action='append',
                        type=str,
                        help='Specify benchmarks to run')

    group.add_argument('-a', '--all', action='store_true', default=False,
                        help='Whether run all the benchmarks')

    # params of perceptron based branch predictor
    parser.add_argument('--bp-size', action='store', type=int,
                        help='Global predictor size')

    parser.add_argument('--bp-index-type', action='store', type=str,
                        help='Indexing method of perceptron BP')

    parser.add_argument('--bp-history-len', action='store', type=int,
                        help='History length(size of each perceptron)')

    parser.add_argument('--bp-learning-rate', action='store', type=int,
                        help='Learning rate of perceptron BP')

    parser.add_argument('--bp-pseudo-tagging', action='store', type=int,
                        help='Num bits of pseudo-tagging')

    parser.add_argument('--bp-dyn-thres', action='store', type=int,
                        help='log2 of num theta used')

    parser.add_argument('--bp-tc-bit', action='store', type=int,
                        help='valid when dyn-thres is not 0, counter bit')

def rv_origin(benchmark, some_extra_args, outdir_b):

    interval = 200*10**6
    warmup = 20*10**6

    os.chdir(c.gem5_exec())

    options = [
            '--outdir=' + outdir_b,
            '--debug-flags=' + debug_flag,
            pjoin(c.gem5_home(), 'configs/spec2006/se_spec06.py'),
            '--spec-2006-bench',
            '-b', '{}'.format(benchmark),
            '--benchmark-stdout={}/out'.format(outdir_b),
            '--benchmark-stderr={}/err'.format(outdir_b),
            '-I {}'.format(220*10**6),
            '--mem-size=4GB',
            '-r 1',
            '--restore-simpoint-checkpoint',
            '--checkpoint-dir={}'.format(pjoin(c.gem5_cpt_dir(arch),
                benchmark)),
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
            #'--debug-flags=Fetch',
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
            '--num-IQ={}'.format(numIQ),
            '--num-LQ=100',
            '--num-SQ=100',
            '--num-PhysReg=256']

        opt = some_extra_args
        if opt.bp_size != None:
            options += ['--bp-size='+opt.bp_size]
        if opt.bp_index_type != None:
            options += ['--bp-index-type='+opt.bp_index_type]
        if opt.bp_history_len != None:
            options += ['--bp-history-len='+opt.bp_history_len]
        if opt.bp_learning_rate != None:
            options += ['--bp-learning-rate='+opt.bp_learning_rate]
        if opt.bp_pseudo_tagging != None and opt.bp_pseudo_tagging != 0:
            options += ['--bp-pesudo-tagging='+opt.bp_pseudo_tagging]
        if opt.bp_dyn_thres != None:
            options += ['--bp-dyn-thres='+opt.bp_dyn_thres]
            if opt.bp_tc_bit != None:
                options += ['--bp-tc-bit='+opt.bp_tc_bit]
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


def run(benchmark, opt):
    outdir_b = pjoin(outdir, benchmark)
    if not os.path.isdir(outdir_b):
        os.makedirs(outdir_b)

    cpt_flag_file = pjoin(c.gem5_cpt_dir(arch), benchmark,
            'ts-take_cpt_for_benchmark')
    prerequisite = os.path.isfile(cpt_flag_file)
    some_extra_args = opt

    if prerequisite:
        print('cpt flag found, is going to run gem5 on', benchmark)
        c.avoid_repeated(rv_origin, outdir_b,
                benchmark, some_extra_args, outdir_b)
    else:
        print('prerequisite not satisified, abort on', benchmark)


def main():
    parser = argparse.ArgumentParser(usage='-n [-s | -a]')

    parser_add_argument(parser)

    opt = parser.parse_args()

    num_thread = opt.num_threads

    benchmarks = []

    if opt.specified_benchmark == None:
        targets = ''
        if opt.all:
            targets = 'all_function_spec.txt'
        else:
            targets = 'target_function_spec.txt'

        with open(targets) as f:
            for line in f:
                benchmarks.append(line.strip())
    else:
        for bench in opt.specified_benchmark:
            benchmarks.append(bench)

    if num_thread > 1:
        p = Pool(num_thread)
        p.map(run, benchmarks, opt)
    else:
        run(benchmarks[0], opt)


if __name__ == '__main__':
    main()

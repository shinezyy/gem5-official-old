#!/usr/bin/env python3

import os
import random
import sh
import operator
from os.path import join as pjoin
from os.path import expanduser as uexp
from multiprocessing import Pool
import common as c


simpoints_all = uexp('~/gem5-results/simpoint-profile')


def get_spec():
    x = []
    with open('./all_compiled_spec.txt') as f:
        for line in f:
            if not line.startswith('#'):
                x.append(line.lstrip('#').strip('\n'))
    return x


def choose_weights(benchmark):
    print("choose weights for", benchmark)
    benchmark_dir = pjoin(simpoints_all, benchmark)
    os.chdir(benchmark_dir)
    max_list = []
    for i in range(10):
        weights = 'weights' + str(i)
        simpoints = 'simpoints' + str(i)

        weights_dict = {}
        cluster_dict = {}

        with open(simpoints) as sf:
            for line in sf:
                vector, n = list(map(int, line.split()))
                cluster_dict[n] = vector

        with open(weights) as wf:
            for line in wf:
                weight, n = list(map(float, line.split()))
                n = int(n)
                weights_dict[cluster_dict[n]] = weight

        sort_by_weights = sorted(iter(weights_dict.items()),
                key=operator.itemgetter(1), reverse=True)
        max_list.append(sort_by_weights[0])

    max_list = sorted(max_list, key=operator.itemgetter(1), reverse=True)
    print(max_list)

    max_of_max = max_list[0]

    with open('simpoints-final', 'w') as sf:
        print(max_of_max[0], 0, file=sf)
    with open('weights-final', 'w') as wf:
        print(1, 0, file=wf)



def cluster(benchmark):
    print('cluster on', benchmark)
    simpoint_file_name = 'simpoint.bb.gz'
    random.seed(os.urandom(8))

    benchmark_dir = pjoin(simpoints_all, benchmark)
    os.chdir(benchmark_dir)
    print(benchmark_dir)

    assert(os.path.isfile(simpoint_file_name))
    assert(os.path.isfile(pjoin(benchmark_dir, simpoint_file_name)))

    for i in range(10):
        weights = 'weights' + str(i)
        simpoints = 'simpoints' + str(i)
        # 10 times of clustering is performed
        sh.simpoint('-loadFVFile', simpoint_file_name,
                    '-saveSimpoints', simpoints,
                    '-saveSimpointWeights', weights,
                    '-inputVectorsGzipped',
                    '-maxK', 30,
                    '-numInitSeeds', 40,
                    '-iters', 100000,
                    '-seedkm', random.randint(0, 2**32 - 1),
                    '-seedproj', random.randint(0, 2**32 - 1),
                    # '-seedsample', random.randint(0, 2**32 - 1),
                   )


def cluster_and_choose(benchmark):
    benchmark_dir = pjoin(simpoints_all, benchmark)
    prerequisite = os.path.isfile(pjoin(benchmark_dir, 'done'))
    if prerequisite:

        print('done file found in {}, is to perform clustering'.format(
            benchmark_dir))
        c.avoid_repeated(cluster, benchmark_dir, benchmark)

        print('clustered, is to choosing according to weights for {}'.format(
            benchmark_dir))
        c.avoid_repeated(choose_weights, benchmark_dir, benchmark)

    else:
        print('done file found in {}, abort'.format(
            benchmark_dir))


if __name__ == '__main__':
    p = Pool(27)
    p.map(cluster_and_choose, get_spec())


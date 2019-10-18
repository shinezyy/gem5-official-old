#!/usr/bin/env python3

import os
import random
import sh
import operator
from os.path import join as pjoin
from os.path import expanduser as uexp
from multiprocessing import Pool
import common as c


# Please set to the directory where gem5-generated bbvs stored
simpoint_profile_dir = '/home/zyy/gem5-results/spec2017_simpoint_profile/'
simpoints_all = simpoint_profile_dir
assert simpoint_profile_dir != 'deadbeaf'

def get_spec():
    x = []
    with open('./all_function_spec2017.txt') as f:
    # with open('./tmp.txt') as f:
        for line in f:
            if not line.startswith('#'):
                x.append(line.lstrip('#').strip('\n'))
    return x


def choose_weights(benchmark):
    print("choose weights for", benchmark)
    benchmark_dir = pjoin(simpoint_profile_dir, benchmark)
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
        print(sort_by_weights)
        l = min(len(sort_by_weights), 3)
        entry = ((*sort_by_weights[:l]), sum([x for _, x in sort_by_weights[:l]]))
        print(entry)
        max_list.append(entry)

    max_list = sorted(max_list, key=operator.itemgetter(3), reverse=True)
    print(max_list)
    chosen = max_list[0][:-1]
    print(chosen)

    with open('simpoints-final', 'w') as sf:
        for i, pair in enumerate(chosen):
            sf.write(f'{pair[0]} {i}\n')
    with open('weights-final', 'w') as wf:
        for i, pair in enumerate(chosen):
            wf.write(f'{pair[1]} {i}\n')



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
        c.avoid_repeated(cluster, benchmark_dir, None, benchmark)

        print('clustered, is to choosing according to weights for {}'.format(
            benchmark_dir))
        c.avoid_repeated(choose_weights, benchmark_dir, None, benchmark)

    else:
        print('done file found in {}, abort'.format(
            benchmark_dir))


if __name__ == '__main__':
    p = Pool(22)
    p.map(cluster_and_choose, get_spec())


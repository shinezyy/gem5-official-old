import os
import sys
from os.path import join as pjoin
import time
import argparse
import re
import codecs
import csv
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from enum import Enum
from math import *

res_dir = '/home/glr/gem5/gem5-results'

unconf_pattern = re.compile(r'\d+%')
alias_pattern  = re.compile(r'\d+!')

total_bench = 22

class Type(Enum):
    unconf = 1
    alias = 2
    nu_ratio = 3

# get the subdirectory last modified
def get_latest_subdir(dir='.'):
    d = os.listdir(dir)
    target_dir = ''
    latest = 0
    for dirs in d:
        if os.path.isdir(dirs):
            stat = os.stat(pjoin(res_dir, dirs))
            if (stat.st_mtime > latest):
                latest = stat.st_ctime
                target_dir = dirs
    print("Latest subdir is", target_dir, '\n')
    return target_dir

# return paths of target output files
# if latest, return only 22 files
# if all, return all groups of 22 files
def get_output(target_str, l):
    res = []

    # only latest ones, default
    if (l):
        # optional, target assigned
        if (target_str != None):
            list_dirs = os.listdir(res_dir)
            for d in list_dirs:
                if target_str == d:
                    list_dirs = os.walk(pjoin(res_dir, d))
                    for root, dirs, files in list_dirs:
                        for f in files:
                            if 'gem5_out.txt' in f:
                                outdir = pjoin(root, f)
                                res.append(outdir)
            print(target_str)
        # default latest
        else:
            latest_subdir = get_latest_subdir(res_dir)
            list_dirs = os.walk(pjoin(res_dir, latest_subdir))
            for root, dirs, files in list_dirs:
                for f in files:
                    if 'gem5_out.txt' in f:
                        outdir = pjoin(root,f)
                        res.append(outdir)
    # all outputs
    else:
        d = os.listdir(res_dir)
        for dir in d:
            test = []
            list_dirs = os.walk(dir)
            for root, dirs, files in list_dirs:
                for f in files:
                    if 'gem5_out.txt' in f:
                        outdir = pjoin(root, f)
                        test.append(outdir)
            res.append(test)

    #print(res)
    return res

def extract_data(dir, pattern):
    count = 0
    interval = 100000

    seq = interval

    out_txt = codecs.open(dir, 'r', encoding='utf-8', errors='ignore')

    matrix = []

    for line in out_txt:
        row = [seq/interval]
        for num in pattern.finditer(line):
            item = num.group()
            item = item[:-1]
            row.append(float(item))

            matrix.append(row)
            seq += interval

    out_txt.close()

    return matrix

def write_csv(dir, data):
    with open(dir, 'w') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerows(data)
    csvfile.close()

def plot(target_dir, data, type):
    fig = plt.figure(figsize=(20,10))
    num_subfigs = 8
    count = 0

    if not os.path.isdir(target_dir):
        os.makedirs(target_dir)

    for i in range(len(data)):
        # per-bench data
        pb_data = data[i]
        [name, mat] = pb_data
        mat = np.array(mat[1:])

        num_h = floor(sqrt(num_subfigs))
        num_w = int(num_subfigs / num_h)
        assert(num_h * num_w >= num_subfigs)

        plt.subplot(num_h, num_w, (count%num_subfigs)+1)
        count += 1

        if type == Type.unconf:
            plt.plot(mat[:,0], mat[:,1], label='< theta',   color='r')
            plt.plot(mat[:,0], mat[:,2], label='< theta/2', color='y')
            plt.plot(mat[:,0], mat[:,3], label='< theta/4', color='b')

            plt.xlabel(name)
            plt.ylabel('percentage')

            plt.legend(loc='best')
        elif type == Type.alias:
            plt.plot(mat[:,0],mat[:,1], label='', color='b')

            plt.xlabel(name)
            plt.ylabel('number')


        if count % num_subfigs == 0 or count == total_bench:
            plt.savefig(pjoin(target_dir, str(count)+'.png'))
            plt.close('all')
            fig = plt.figure(figsize=(20,10))

def per_test_process(out_dirs, type):
    data = []
    target_dir = ''

    first_row = []
    pattern = unconf_pattern
    issue = ''
    if type == Type.unconf:
        first_row = ['', 'less_than_theta', 'less_than_theta/2',\
                'less_than_theta/4']
        pattern = unconf_pattern
        issue = 'unconf'
    elif type == Type.alias:
        first_row = ['', '']
        pattern = alias_pattern
        issue = 'alias'

    for d in out_dirs:

        matrix = extract_data(d, pattern)
        matrix.insert(0, first_row)

        p = d.split('/')
        function = p[-2]
        test     = p[-3]

        target_dir = pjoin(res_dir, test, 'res', issue)
        if not os.path.isdir(target_dir):
            os.makedirs(target_dir)
        #print(target_dir, 'is target output dir\n')
        write_csv(pjoin(target_dir, function+'.csv'), matrix)

        data.append([function, matrix])

    plot(target_dir, data, type)

def get_unconfident_percent(target_str=None, latest=True):
    pattern = unconf_pattern
    out_dirs = get_output(target_str, l=latest)
    interval = 100000

    if latest:
        per_test_process(out_dirs, Type.unconf)
    else:
        for test in out_dirs:
            per_test_process(test, Type.unconf)


def get_num_aliasing(target_str=None, latest=True):
    pattern = alias_pattern
    out_dirs = get_output(target_str, l=latest)
    interval = 100000

    if latest:
        per_test_process(out_dirs, Type.alias)
    else:
        for test in out_dirs:
            per_test_process(test, Type.alias)

def main():

    parser = argparse.ArgumentParser(usage='test of argparse')
    parser.add_argument('-a', '--aliasing', action='store_true',
                        help='get output result of aliasing')
    parser.add_argument('-u', '--unconfident', action='store_true',
                        help='get output result of unconfident rate')
    parser.add_argument('-d', '--specified-directory', action='store',
                        help = 'specify the result dir')
    parser.add_argument('-e', '--entire', action='store_true',
                        default=False, help='get all results in the folder')

    args = parser.parse_args()
    dir = args.specified_directory
    latest = not args.entire

    print(latest)

    if (args.aliasing):
        get_num_aliasing(dir, latest)
    if (args.unconfident):
        get_unconfident_percent(dir, latest)

if __name__ == '__main__':
    main()

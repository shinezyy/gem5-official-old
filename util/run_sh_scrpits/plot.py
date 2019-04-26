#!/usr/bin/env python3.6
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

home = os.getenv('HOME')

res_dir = pjoin(home,'gem5/gem5-results')

unconf_pattern = re.compile(r'\d+%')
alias_pattern  = re.compile(r'\d+!')
usage_pattern  = re.compile(r'\d+#')
dalias_pattern = re.compile(r'\d+@')

total_bench = 22

class Type(Enum):
    unconf = 1
    alias = 2
    nu_ratio = 3
    usage = 4
    dalias = 5

def reverse_readline(filename: str, buf_size=16384):
    """a generator that returns the lines of a file in reverse order"""
    with open(filename) as fh:
        segment = None
        offset = 0
        fh.seek(0, os.SEEK_END)
        file_size = remaining_size = fh.tell()
        while remaining_size > 0:
            offset = min(file_size, offset + buf_size)
            fh.seek(file_size - offset)
            buffer = fh.read(min(remaining_size, buf_size))
            remaining_size -= buf_size
            lines = buffer.split('\n')
            # the first line of the buffer is probably not a complete line so
            # we'll save it and append it to the last line of the next buffer
            # we read
            if segment is not None:
                # if the previous chunk starts right from the beginning of line
                # do not concact the segment to the last line of new chunk
                # instead, yield the segment first
                if buffer[-1] is not '\n':
                    lines[-1] += segment
                else:
                    yield segment
            segment = lines[0]
            for index in range(len(lines) - 1, 0, -1):
                if len(lines[index]):
                    yield lines[index]
        # Don't yield None if the file was empty
        if segment is not None:
            yield segment

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
            if len(test):
                res.append(test)

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

def get_bp_size(dir):
    ind = dir.find("my")
    #print("my is at index", ind)
    size = int(dir[ind+7:ind+11])
    #print("bp_size is ", size)
    return size

def extract_data_usage(dir, pattern):
    i = 0
    matrix = []

    bp_size = get_bp_size(dir)

    print('Extract usage data from ', dir)
    for line in reverse_readline(dir):
        row = [bp_size - i]
        all_matches = pattern.findall(line)
        for item in pattern.findall(line):
            item = item[:-1]
            row.append(float(item))

            matrix.append(row)
            i += 1
        if len(matrix) > bp_size - 1:
            break
    return matrix

def write_csv(dir, data):
    with open(dir, 'w') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerows(data)
    csvfile.close()

def plot(target_dir, data, type):
    print('plotting target dir is' , target_dir)
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
#
            plt.xlabel(name)
            plt.ylabel('percentage')

            plt.legend(loc='best')
        elif type == Type.alias:
            plt.plot(mat[:,0],mat[:,1], label='', color='b')

            plt.xlabel(name)
            plt.ylabel('number')
        elif type == Type.dalias:
            plt.plot(mat[:,0],mat[:,1], label='', color='b')

            plt.xlabel(name)
            plt.ylabel('number')
        elif type == Type.usage:
            plt.bar(range(len(mat[:,1])), np.sort(mat[:,1])[::-1], color='b')

            plt.xlabel(name)
            plt.ylabel('lookups')


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
    elif type == Type.dalias:
        first_row = ['','']
        pattern = dalias_pattern
        issue = 'dalias'
    elif type == Type.usage:
        first_row = ['', '']
        pattern = usage_pattern
        issue = 'usage'

    for d in out_dirs:
        matrix = []

        # Too ugly
        if type != Type.usage:
            matrix = extract_data(d, pattern)
        else:
            matrix = extract_data_usage(d, pattern)
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
    out_dirs = get_output(target_str, l=latest)
    interval = 100000

    if latest:
        per_test_process(out_dirs, Type.unconf)
    else:
        for test in out_dirs:
            per_test_process(test, Type.unconf)


def get_num_aliasing(target_str=None, latest=True):
    out_dirs = get_output(target_str, l=latest)
    interval = 100000

    if latest:
        per_test_process(out_dirs, Type.alias)
    else:
        for test in out_dirs:
            per_test_process(test, Type.alias)

def get_num_daliasing(target_str=None, latest=True):
    out_dirs = get_output(target_str, l=latest)
    interval = 100000

    if latest:
        per_test_process(out_dirs, Type.dalias)
    else:
        for test in out_dirs:
            per_test_process(test, Type.dalias)

def get_table_usage(target_str=None, latest=True):
    out_dirs = get_output(target_str, l=latest)
    interval = 100000

    if latest:
        per_test_process(out_dirs, Type.usage)
    else:
        for test in out_dirs:
            per_test_process(test, Type.usage)

def process(args, dir, latest):
    if (args.aliasing):
        get_num_aliasing(dir, latest)
    if (args.unconfident):
        get_unconfident_percent(dir, latest)
    if (args.table_usage):
        get_table_usage(dir, latest)
    if (args.destructive_aliasing):
        get_num_daliasing(dir, latest)


def main():

    parser = argparse.ArgumentParser(usage='test of argparse')
    parser.add_argument('-a', '--aliasing', action='store_true',
                        help='get output result of aliasing')
    parser.add_argument('-d', '--destructive-aliasing', action='store_true',
                        help='get output result of destructive aliasing')
    parser.add_argument('-u', '--unconfident', action='store_true',
                        help='get output result of unconfident rate')
    parser.add_argument('-t', '--table-usage', action='store_true',
                        help='get output result of table usage')
    parser.add_argument('-s', '--specified-directory', action='append',
                        help = 'specify the result dir')
    parser.add_argument('-e', '--entire', action='store_true',
                        default=False, help='get all results in the folder')
    parser.add_argument('--no-plot', action='store_true',
                        help='Do not plot')

    args = parser.parse_args()
    dir = args.specified_directory
    latest = not args.entire

    print(latest)

    if args.specified_directory:
        for dir in args.specified_directory:
            process(args, dir, latest)
    else:
        process(args, dir, latest)

if __name__ == '__main__':
    main()

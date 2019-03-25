import os
import sys
import re
import codecs
import csv
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from math import *

def get_output(dir, target_str):
    list_dirs = os.walk(dir)
    res = []
    for root, dirs, files in list_dirs:
        for f in files:
            outdir = os.path.join(root,f)
            if 'gem5_out.txt' in outdir and target_str in outdir:
                res.append(outdir)
    return res

def get_unconfident_percent(target_str='debug'):
    pattern = re.compile(r'\d+%')
    out_dirs = get_output('.', target_str)
    interval = 100000

    count = 0;
    fig = plt.figure(figsize=(20,10))
    num_subfigs = 8

    for d in out_dirs:
        matrix = [['', 'less_than_theta', 'less_than_theta/2',\
                    'less_than_theta/4']]
        seq = interval

        out_txt = codecs.open(d, "r", encoding='utf-8',errors='ignore')
        for line in out_txt:
            #line = line.decode('unicode_escape').encode('utf-8')
            #print(line)
            if "During" in line:
                row = [seq/interval]
                seq += interval
                for num in pattern.finditer(line):
                    #print(num.group())
                    item = num.group()
                    item = item[:-1]
                    row.append(float(item))
                matrix.append(row)
        out_txt.close()

        p = d.split('/')
        #print(p)
        function = p[-2]

        with open(os.path.join('./data', target_str, \
                function+'.csv'), 'w') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerows(matrix)

        csvfile.close()

       # fig = plt.figure(0)
        #print(np.array(matrix).shape)
        matrix = np.array(matrix[1:])

        num_h = floor(sqrt(num_subfigs))
        num_w = int(num_subfigs / num_h)
        assert(num_h * num_w >= num_subfigs)
        plt.subplot(num_h, num_w, (count%num_subfigs)+1)
        count+=1

        plt.plot(matrix[:,0],matrix[:,1], label='sum<theta',   color='red')
        plt.plot(matrix[:,0],matrix[:,2], label='sum<theta/2', color='yellow')
        plt.plot(matrix[:,0],matrix[:,3], label='sum<theta/4', color='blue')

        plt.xlabel(function)
        plt.ylabel('percentage')

        plt.legend(loc='best')

#        plt.xticks(rotation=70)
        if count % num_subfigs == 0 or count == 22:
            plt.savefig(os.path.join('data', target_str, \
                    str(count/num_subfigs)+'.png'))
            plt.close('all')
            fig = plt.figure(figsize=(20,10))

def get_num_aliasing(target_str='alias'):
    pattern = re.compile(r'\d+!')
    out_dirs = get_output('.', target_str)
    interval = 100000

    count = 0;
    fig = plt.figure(figsize=(20,10))
    num_subfigs = 8

    for d in out_dirs:
        matrix = [['', '']]
        seq = interval

        out_txt = codecs.open(d, "r", encoding='utf-8',errors='ignore')

        temp = 0

        for line in out_txt:
            #line = line.decode('unicode_escape').encode('utf-8')
            #print(line)
            if "Lookup" in line:
                row = [seq/interval]
                seq += interval
                for num in pattern.finditer(line):
                    #print(num.group())
                    item = num.group()
                    item = item[:-1]
                    row.append(float(item) - temp)
                    #print(float(item))
                    #print(temp)
                    #print(float(item)-temp)
                    temp = float(item)
                    #print('temp is', temp)
                matrix.append(row)
        out_txt.close()

        p = d.split('/')
        #print(p)
        function = p[-2]

        with open(os.path.join('./data',\
              target_str, function+'.csv'), 'w') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerows(matrix)

        csvfile.close()

       # fig = plt.figure(0)
        print(function, np.array(matrix).shape)

        matrix = np.array(matrix[1:])

        num_h = floor(sqrt(num_subfigs))
        num_w = int(num_subfigs / num_h)
        assert(num_h * num_w >= num_subfigs)
        plt.subplot(num_h, num_w, (count%num_subfigs)+1)
        count+=1

        plt.plot(matrix[:,0],matrix[:,1], label='', color='b')

        plt.xlabel(function)
        plt.ylabel('number')

        #plt.legend(loc='best')

#        plt.xticks(rotation=70)
        if count % num_subfigs == 0 or count == 22:
            plt.savefig(os.path.join('data', target_str,\
                    str(count/num_subfigs)+'.png'))
            plt.close('all')
            fig = plt.figure(figsize=(20,10))

def main():
    if sys.argv[1] == 'a':
        if (sys.argv[2]):
            get_num_aliasing(sys.argv[2])
        else:
            get_num_aliasing()
    elif sys.argv[1] == 'c':
        if (sys.atgv[2]):
            get_unconfident_percent(sys.argv[2])
        else:
            get_unconfident_percent()

if __name__ == '__main__':
    main()

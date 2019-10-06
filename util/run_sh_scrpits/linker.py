import sh
import os
from os.path import join as pjoin

base = os.environ['cpu_2017_dir']

file_paths = []
files = []

output_dir = '/home/zyy/cpu2017_run_dir'

for d in os.listdir(base):
    if os.path.isfile(pjoin(base, d)):
        continue
    if d.endswith('_s'):
        continue
    data_dir = pjoin(base, d, 'data')
    bmk = d.split('.')[1].split('_')[0]
    bmk_dir = pjoin(output_dir, bmk)

    if not os.path.isdir(bmk_dir):
        os.makedirs(bmk_dir)

    ref_data = pjoin(data_dir, 'refrate', 'input')
    if os.path.isdir(ref_data):
        for f in os.listdir(ref_data):
            sh.cp(['-r', pjoin(ref_data, f), bmk_dir])

    all_data = pjoin(data_dir, 'all', 'input')
    if os.path.isdir(all_data):
        for f in os.listdir(all_data):
            sh.cp(['-r', pjoin(all_data, f), bmk_dir])




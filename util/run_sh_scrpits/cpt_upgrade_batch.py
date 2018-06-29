from os.path import join as pjoin
import os
import sh

cmd = sh.Command('../cpt_upgrader.py')


benchmarks = []

with open('./all_function_spec.txt') as f:
    for line in f:
        benchmarks.append(line.strip())

paths = []
for b in benchmarks:
    b_path = pjoin('/home/share/st_checkpoint', b)
    for d in os.listdir(b_path):
        if d.startswith('cpt'):
            cpt_path = pjoin(b_path, d)
            assert os.path.isdir(cpt_path)
            paths.append(cpt_path)

for p in paths:
    print(p)
    cmd(p)

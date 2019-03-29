#!/usr/bin/env python3
import os
import shutil

res_dir = "/home/glr/gem5/gem5-results"

# Removes the latest result directory when called
def main():
    d = os.listdir(res_dir)
    target_dir = ''
    latest = 0
    for dirs in d:
        stat = os.stat(os.path.join(res_dir, dirs))
        if (stat.st_mtime > latest):
            latest = stat.st_mtime
            target_dir = dirs
        #print(dirs,"\n", stat.st_mtime)

    print("Latest dir is\n", target_dir)

    target = os.path.join(res_dir, target_dir)
    shutil.rmtree(target)

if __name__ == '__main__':
    main()

import m5
import os
import sys
from m5.objects import *

from parse_inputs import get_input_dict
from os.path import join as pjoin

# These three directory paths are not currently used.
# Add a new environment variable in your bashrc: cpu_2006_dir=<CPU2006 path>


class Spec17:
    def __init__(self, suffix, size):
        print 'Initializing Spec17 Object'
        self.edu_flag = self.init_env()
        self.cmd_map, self.name_map = get_input_dict()

        self.spec_dir = os.environ['cpu_2017_dir']+'/'
        self.pid_idx = 0
        self.size = size
        self.suffix = suffix


    def init_env(self):
        if 'spec_version' in os.environ:
            if os.environ['spec_version'] == 'education':
                return True
        return False

    def skip(self, s):
        nofile = ['<', '-', '.', 'gtp', 'reference.dat',
                  'namd.out', '166.s',
                  '19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e',]
        if s.isdigit():
            return True
        for p in nofile:
            if s.startswith(p):
                return True
        return False

    def gen_proc(self, benchmark_name):
        proc = Process(pid = 100 + self.pid_idx)
        self.pid_idx += 1
        if not benchmark_name in self.name_map:
            return None

        full_name = self.name_map[benchmark_name]
        cmds = self.cmd_map[benchmark_name]

        exe_sub_dir = '{}/exe/{}{}'.format(full_name, cmds[0][0].split('_r')[0],
                self.suffix)
        executable = pjoin(self.spec_dir, exe_sub_dir)
        proc.executable = executable

        if self.size == 'ref':
            d = 'refrate'
        else:
            d = self.size

        ref_input_dir = pjoin(self.spec_dir,
                '{}/data/{}/input/'.format(full_name, d))

        all_input_dir = pjoin(self.spec_dir,
                '{}/data/all/input/'.format(full_name))

        first_cmd_list = cmds[0]

        if benchmark_name == 'perlbench':
            cmds = first_cmd_list[2:]
            for i, c in enumerate(cmds):
                if not self.skip(c):
                    cmds[i] = pjoin(ref_input_dir, c)
                    #assert(os.path.isfile(cmds[i]))
            proc.cmd = [executable] + ['-I{}lib'.format(all_input_dir)] + cmds

        elif benchmark_name == 'namd':
            cmds = first_cmd_list[1:]
            for i, c in enumerate(cmds):
                if not self.skip(c):
                    cmds[i] = pjoin(all_input_dir, c)
                    #assert(os.path.isfile(cmds[i]))
            proc.cmd = [executable] + cmds

        else:
            cmds = first_cmd_list[1:]
            skip = False
            for i, c in enumerate(cmds):
                if skip:
                    skip = False
                    pass
                if not self.skip(c):
                    inp = pjoin(ref_input_dir, c)
                    if os.path.isfile(inp):
                        cmds[i] = inp
                        continue
                    inp = pjoin(all_input_dir, c)
                    print inp
                    if os.path.isfile(inp):
                        cmds[i] = inp

                elif '<' == c:
                    skip = True
                    proc.input = pjoin(ref_input_dir, cmds[i+1])
                    print 'stdin:', proc.input
                    cmds = cmds[:-2]
                    break
            proc.cmd = [executable] + cmds

        print 'cmd:', proc.cmd
        return proc


if __name__ == '__main__':
    spec = Spec06()
    for k in spec.name_map:
        spec.gen_proc(k)

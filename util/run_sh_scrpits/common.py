import os
import sh
from os.path import join as pjoin


def avoid_repeated(func, outdir, binary=None, *args, **kwargs):
    func_name = func.__name__

    running_lock_name = 'running-{}'.format(func_name)
    running_lock_file = pjoin(outdir, running_lock_name)

    if os.path.isfile(running_lock_file):
        print('running lock found in {}, skip!'.format(outdir))
        return
    else:
        sh.touch(running_lock_file)

    ts_name = 'ts-{}'.format(func_name)
    out_ts_file = pjoin(outdir, ts_name)
    if os.path.isfile(out_ts_file):
        out_ts = os.path.getmtime(out_ts_file)
    else:
        out_ts = 0.0

    script_dir = os.path.dirname(os.path.realpath(__file__))
    script_ts_file = pjoin(script_dir, ts_name)
    if not os.path.isfile(script_ts_file):
        sh.touch(script_ts_file)

    newest = os.path.getmtime(script_ts_file)

    if not binary is None:
        binary_ts = os.path.getmtime(binary)
        newest = max(binary_ts, newest)


    if out_ts < newest:
        try:
            func(*args, **kwargs)
            sh.touch(out_ts_file)
        except Exception as e:
            print(e)
        sh.rm(running_lock_file)
    else:
        print('{} is older than {}, skip!'.format(out_ts_file,
            script_ts_file))
        if os.path.isfile(running_lock_file):
            sh.rm(running_lock_file)


def gem5_home():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    paths = script_dir.split('/')
    return '/'.join(paths[:-2])  # delete '/util/run_sh_scrpits'


def gem5_build(arch):
    return pjoin(gem5_home(), 'build/{}'.format(arch))


def gem5_exec(spec_version = '2006'):
    if spec_version == '2006':
        return os.environ['gem5_run_dir']
    elif spec_version == '2017':
        return os.environ['spec2017_run_dir']
    return None

def gem5_cpt_dir(arch, version=2006):
    cpt_dirs = {
            2006: {
                'ARM': '/ramdisk/zyy/gem5_run/spec-simpoint-cpt-arm-gcc-4.8',
                'RISCV': '/home/zyy/spec-simpoint-cpt-riscv-gcc-8.2',
                },
            2017: {
                'ARM': None,
                'RISCV': '/home/zyy/gem5-results/spec2017_simpoint_cpts_finished',
                },
            }
    return cpt_dirs[version][arch]

stats_base_dir = '/home/zyy/gem5-results-2017'


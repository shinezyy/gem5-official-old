import os
import sh
from os.path import join as pjoin


def avoid_repeated(func, outdir, *args, **kwargs):
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
    script_ts = os.path.getmtime(script_ts_file)

    if out_ts < script_ts:
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


def gem5_build():
    return pjoin(gem5_home(), 'build/ARM')


def gem5_exec():
    return os.environ['gem5_run_dir']

import os
import sh
from os.path import join as pjoin


def avoid_repeated(func, outdir, *args, **kwargs):
    func_name = func.__name__
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
        func(*args, **kwargs)
        sh.touch(out_ts_file)
    else:
        print('{} is older than {}, skipped!'.format(out_ts_file,
            script_ts_file))


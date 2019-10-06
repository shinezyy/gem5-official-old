import re
import os

def get_input_dict_file(file_name):
    text = ''
    file_dir = os.path.dirname(os.path.realpath(__file__))
    with open(os.path.join(file_dir, file_name)) as f:
        text = f.read()

    title = re.compile('# ((\d{3})\.(\w+)) \w+')

    # current key
    cmds = {}
    cur_bmk = None
    name_dict = {}

    for line in text.split('\n'):
        m = title.match(line)
        if len(line) < 3:
            continue
        if m:
            cur_bmk = m.group(3).split('_')[0]
            cmds[cur_bmk] = []
            name_dict[cur_bmk] = m.group(1)
            continue
        cmd = re.split('\s+', line.split('>')[0].strip())
        cmds[cur_bmk].append(cmd)

    return cmds, name_dict

def get_input_dict(t = 'all', size = 'ref'):
    if t == 'int' or t == 'fp':
        return get_input_dict_file('./spec2017-{}rate-{}.txt'.format(t, 'ref'))
    else:
        assert t == 'all'
        all_bmks, all_names = \
                get_input_dict_file('./spec2017-{}rate-{}.txt'.format('int', 'ref'))
        fp_bmks, fp_names = \
                get_input_dict_file('./spec2017-{}rate-{}.txt'.format('fp', 'ref'))
        for k in fp_bmks: # merge into int bmks
            all_bmks[k] = fp_bmks[k]
            all_names[k] = fp_names[k]
        return all_bmks, all_names

if __name__ == '__main__':
    cmd_map, name_dict = get_input_dict(t='all', size='ref')
    for k, v in cmd_map.items():
        print k, v
    for k in name_dict:
        print k



# Usage:
# - copy or link all required inputs to $gem5_take_simponit_dir;
# - copy or link this script to $gem5_take_simponit_di;
# - set $gem5_root;
# - run this script

$gem5_root/util/create_simpoint.sh -b "GemsFDTD" -o $gem5_take_simponit_dir/GemsFDTD -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "hmmer" -o $gem5_take_simponit_dir/hmmer -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "sphinx3" -o $gem5_take_simponit_dir/sphinx3 -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "gobmk" -o $gem5_take_simponit_dir/gobmk -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "cactusADM" -o $gem5_take_simponit_dir/cactusADM -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "tonto" -o $gem5_take_simponit_dir/tonto -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "bwaves" -o $gem5_take_simponit_dir/bwaves -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "lbm" -o $gem5_take_simponit_dir/lbm -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "wrf" -o $gem5_take_simponit_dir/wrf -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "bzip2" -o $gem5_take_simponit_dir/bzip2 -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "leslie3d" -o $gem5_take_simponit_dir/leslie3d -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "gromacs" -o $gem5_take_simponit_dir/gromacs -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "sjeng" -o $gem5_take_simponit_dir/sjeng -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "calculix" -o $gem5_take_simponit_dir/calculix -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "astar" -o $gem5_take_simponit_dir/astar -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "zeusmp" -o $gem5_take_simponit_dir/zeusmp -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "povray" -o $gem5_take_simponit_dir/povray -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "omnetpp" -o $gem5_take_simponit_dir/omnetpp -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "mcf" -o $gem5_take_simponit_dir/mcf -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "gcc" -o $gem5_take_simponit_dir/gcc -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "h264ref" -o $gem5_take_simponit_dir/h264ref -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "perlbench" -o $gem5_take_simponit_dir/perlbench -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "xalancbmk" -o $gem5_take_simponit_dir/xalancbmk -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "dealII" -o $gem5_take_simponit_dir/dealII -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "namd" -o $gem5_take_simponit_dir/namd -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "libquantum" -o $gem5_take_simponit_dir/libquantum -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "gamess" -o $gem5_take_simponit_dir/gamess -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "soplex" -o $gem5_take_simponit_dir/soplex -a ARM -c simpoint.py -v fast
$gem5_root/util/create_simpoint.sh -b "milc" -o $gem5_take_simponit_dir/milc -a ARM -c simpoint.py -v fast

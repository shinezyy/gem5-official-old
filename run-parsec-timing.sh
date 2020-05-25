./build/X86/gem5.opt \
--outdir=$2/timing_$1t \
./configs/example/fs.py -n 8 \
--script=benchmarks/$2_$1c_simdev.rcS \
--mem-type=DDR3_1600_8x8 \
--cpu-type=TimingSimpleCPU \
--caches \
--cacheline_size=64 \
--l1i_size=32kB \
--l1d_size=32kB \
--l1i_assoc=4 \
--l1d_assoc=4 \
--l2cache \
--l2_size=8MB \
--l2_assoc=8 \
--kernel=parsec_images/system/binaries/x86_64-vmlinux-2.6.28.4-smp
# -s 1 \
# -r 1 \

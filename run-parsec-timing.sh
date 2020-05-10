./build/X86/gem5.opt --outdir=timing_out \
./configs/example/fs.py -n 4 \
-F 255477752 \
--script=./benchmarks/vips_4c_simdev.rcS \
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

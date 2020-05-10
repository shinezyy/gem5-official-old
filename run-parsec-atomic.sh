./build/X86/gem5.opt --outdir=atomic_out \
./configs/example/fs.py -n 4 \
-F 655477752 \
--script=./benchmarks/vips_4c_simdev.rcS \
--kernel=parsec_images/system/binaries/x86_64-vmlinux-2.6.28.4-smp

$gem5_build/gem5.opt \
--outdir=/tmp/bzip2 \
$gem5_root/configs/spec2006/se_spec06.py \
-c \
--mem-size=8GB \
--spec-2006-bench \
-b bzip2 \
--benchmark-stdout=/tmp/bzip2/out \
--benchmark-stderr=/tmp/bzip2/err \
--cpu-type=DerivO3CPU \
--caches \
--cacheline_size=64 \
--l1i_size=32kB \
--l1d_size=32kB \
--l1i_assoc=8 \
--l1d_assoc=8 \
--l2cache \
--l2_size=4MB \
--l2_assoc=8


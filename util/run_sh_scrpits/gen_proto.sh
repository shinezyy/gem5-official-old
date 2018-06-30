output_dir=/tmp/bzip2

xxxx/gem5.opt \
--outdir=$output_dir \
xxxx/configs/spec2006/se_spec06.py \
--mem-size=8GB \
--spec-2006-bench \
-b bzip2 \
--benchmark-stdout=$output_dir/out \
--benchmark-stderr=$output_dir/err \
--cpu-type=DerivO3CPU \
--caches \
--cacheline_size=64 \
--l1i_size=32kB \
--l1d_size=32kB \
--l1i_assoc=1 \
--l1d_assoc=1 \
-I 200000000 \
--elastic-trace-en \
--data-trace-file=deptrace.proto.gz \
--inst-trace-file=fetchtrace.proto.gz \
--mem-type=SimpleMemory


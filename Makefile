task%:
	./run-parsec-ooo.sh $* ./benchmarks/vips_$*c_simdev.rcS

run: task1 task2 task4 task8
	echo "End"

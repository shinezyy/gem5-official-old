bench?=none
core?=none
script=./run-parsec-$(core).sh

task%:
	$(script) $* $(bench)

task_cpt%:
	./ooo-cpt.sh $* $(bench)

run: task1 task2 task4 task8
	echo "End"

cpts: task_cpt1 task_cpt2 task_cpt4 task_cpt8
	echo "End"

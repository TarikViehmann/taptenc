#! /bin/bash
#
# household_benchark.bash
# Author: 2021 Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>

if [[ -z "${TFD_DIR}" ]]; then
	# This script uses Temporal Fast Downward to plan on the household domain.
	# The environment variable TFD_DIR has to direct to the directory where
	# executable of the planner resides.
  echo "Variable TFD_DIR not set. abort."
	exit -1
else
  PLANNER="${TFD_DIR}"
fi
path=$PWD/../src/benchmarks/pddl/household
taptenc_path=$PWD
skip_direct_planning=0

for file in ${path}/problem_*;
  do echo "Processing: " $file;
	cd ${PLANNER}
	testno=${file##*/}
	testno=${testno#*_}
	testno=${testno%.pddl}
	solution_folder=${taptenc_path}/household${testno}
	mkdir -p ${taptenc_path}/household${testno}
	solution=${solution_folder}/solution_${testno}.txt
	planout=${solution_folder}/tfd_output_${testno}.txt
	taptencout=${solution_folder}/taptenc_output_${testno}.txt
	touch ${solution}
	if timeout 60 ${PLANNER}/plan ${path}/domain_strips_no_platform.pddl ${file} ${solution} > $planout;
		then
			cd ${taptenc_path}
			${taptenc_path}/household ${solution} ${solution_folder}/transformed_${testno}.txt > $taptencout;
			mv ${taptenc_path}/merged* ${solution_folder}/
		else
			# once the simple domain times out, we can abort the test.
			# all further domains are timing out
			break;
	fi
	cd ${PLANNER}
	solution=${solution_folder}/direct_solution_${testno}.txt
	planout=${solution_folder}/direct_tfd_output_${testno}.txt
	echo $skip_direct_planning
	if [ $skip_direct_planning == 1 ] || ! timeout 60 ${PLANNER}/plan ${path}/domain_strips.pddl ${file} ${solution} > $planout ;
	then
			# no need to plan in the complex domain on bigger instances
			# when it already timed out on smaller ones
			skip_direct_planning=1
	fi
done;

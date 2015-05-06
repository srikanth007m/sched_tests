################################################################################
# Copyright (C) 2015 ARM Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

# Import test suite definitions
source ../../../../init_env

. $TOOLS/functestlib.sh
echo "This scenario does two simultaneous multi-threaded runs of sysbench"
echo "The expectation is that the scheduler will use all cpus when there is"
echo "more load than the big cpus can handle and as the threads finishes"
echo "the remaining threads will be migrated to the big cpus to get best"
echo "possible performance."
echo "ASSUMPTION: number of little cpus >= number of big cpus"

SYSBENCH=$TOOLS/sysbench
TASKSET=$TOOLS/taskset

#TASKSET=taskset
getbigcpucount
BIG_CPU_COUNT=$__RET;

getlittlecpucount
LITTLE_CPU_COUNT=$__RET;

TOTAL_CPUS=$(($BIG_CPU_COUNT+$LITTLE_CPU_COUNT))

getbigcpumask
CPU_FAST="0x$__RET";

getlittlecpumask
CPU_SLOW="0x$__RET";

SYSBENCH_MAXREQS=1000
SYSBENCH_THREADS=$BIG_CPU_COUNT # Must be = number of big cpus
SYSBENCH_RUN="$SYSBENCH --max-requests=$SYSBENCH_MAXREQS --num-threads=$SYSBENCH_THREADS --test=cpu run"

# First run: On fast cpus only
echo $TASKSET $CPU_FAST $SYSBENCH_RUN
$TASKSET $CPU_FAST $SYSBENCH_RUN > tmp_run_fast

# Second run: On slow cpus only
echo $TASKSET $CPU_SLOW $SYSBENCH_RUN
$TASKSET $CPU_SLOW $SYSBENCH_RUN > tmp_run_slow

# Second run: All cpus
echo $SYSBENCH_RUN
$SYSBENCH_RUN > tmp_run_00 &
PID0=$!
echo $SYSBENCH_RUN
$SYSBENCH_RUN > tmp_run_01 &
PID1=$!
wait $PID0
wait $PID1

RESFAST=`cat tmp_run_fast | grep "total time:" | awk '{print $3}' | sed 's/s//'`
RESSLOW=`cat tmp_run_slow | grep "total time:" | awk '{print $3}' | sed 's/s//'`
RES0=`cat tmp_run_00 | grep "total time:" | awk '{print $3}' | sed 's/s//'`
RES1=`cat tmp_run_01 | grep "total time:" | awk '{print $3}' | sed 's/s//'`

PERF_SCALE=`echo "scale=3; $RESSLOW / $RESFAST" | $TOOLS/bc`

# Performance worst case target assuming that the fast cpus are fully utilized:
#    Target = RESFAST + (RESSLOW - RESFAST)/PERF_SCALE
# In this case the slow cpus are idling while the last job is finishing. In this
# scenario the execution time of the two jobs should be close to target and RESFAST.
#
# Ideally, we would like to have:
#    Target_ideal = (2 * RESSLOW)/(PERF_SCALE + 1)
# for both jobs. This is only achieveable if both jobs are switched between the
# fast and slow cpus during execution to give similar boost to both jobs.
#
# The goal is to have the fast cpus busy until both jobs are done. Hence, the last
# job should be running exclusively on the fast cpus after the other job has finished.
# Calculating the execution time of the last task if it could have perfectly balanced
# its work across all cpus, we should get an execution time close to target_ideal iff
# the fast cpus have been fully utilized.
#
# Assuming RES1 finishes last (otherwise replace 0 with 1):
#    RESBAL = RES0 + (RES1 - RES0)*PERF_SCALE/(PERF_SCALE + 1)
#
# So the success criteria is: RESBAL < Target_ideal + error_margin


#TARGET=`echo "scale=3; $RESFAST + ($RESSLOW - $RESFAST)/$PERF_SCALE" | $TOOLS/bc`
TARGET_IDEAL=`echo "scale=3; $RESSLOW * 2 /($PERF_SCALE + 1)" | $TOOLS/bc`

MIN=`echo "if ($RES0 < $RES1) {print $RES0} else {print $RES1}; print \"\n\"" | $TOOLS/bc`
MAX=`echo "if ($RES0 > $RES1) {print $RES0} else {print $RES1}; print \"\n\"" | $TOOLS/bc`

RESBAL=`echo "scale=3; $MIN + ($MAX - $MIN)*$PERF_SCALE/($PERF_SCALE + 1)" | $TOOLS/bc`

RESULT=`echo "!($RESBAL < $TARGET_IDEAL * 1.03)" | $TOOLS/bc` # 3% error margin

echo "RunFast: " $RESFAST
echo "RunSlow: " $RESSLOW
echo "Perf scale: " $PERF_SCALE
echo "Run0: " $RES0
echo "Run1: " $RES1

echo "Min: " $MIN
echo "Max: " $MAX
echo "Resbal: " $RESBAL
echo "Target_ideal: " $TARGET_IDEAL

rm tmp*

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT








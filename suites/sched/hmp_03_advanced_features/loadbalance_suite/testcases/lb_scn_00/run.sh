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
echo "This scenario does two multi-threaded runs of sysbench:"
echo "One with affinity set to the fast cpus, and"
echo "one with default affinity (all)."
echo "A performance improvement is expected in the second case"
echo "as the scheduler can use the slower cpus as well."

PERF_IMPROVEMENT="0.8" # 20% short execution time

SYSBENCH=$TOOLS/sysbench
TASKSET=$TOOLS/taskset

getbigcpucount
BIG_CPU_COUNT=$__RET;

getlittlecpucount
LITTLE_CPU_COUNT=$__RET;

TOTAL_CPUS=$(($BIG_CPU_COUNT+$LITTLE_CPU_COUNT))

getbigcpumask
CPU_FAST="0x$__RET";

SYSBENCH_MAXREQS=1000
SYSBENCH_THREADS=$TOTAL_CPUS # Must be: Total number of cpus >= THREADS > number of fast cpus
SYSBENCH_RUN="$SYSBENCH --max-requests=$SYSBENCH_MAXREQS --num-threads=$SYSBENCH_THREADS --test=cpu run"

# First run: On fast cpus only
echo $TASKSET $CPU_FAST $SYSBENCH_RUN
$TASKSET $CPU_FAST $SYSBENCH_RUN > tmp_run_00


# Second run: All cpus
echo $SYSBENCH_RUN
$SYSBENCH_RUN > tmp_run_01


RES0=`cat tmp_run_00 | grep "total time:" | awk '{print $3}' | sed 's/s//'`
RES1=`cat tmp_run_01 | grep "total time:" | awk '{print $3}' | sed 's/s//'`

RESULT=`echo "$RES0 * $PERF_IMPROVEMENT < $RES1" | $TOOLS/bc`

echo "Run0: " $RES0
echo "Run1: " $RES1

rm tmp*

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT








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

# Check presence & operation of big.LITTLE MP Scheduler
# 
# Run as root to allow task killing after testing.
#

source $TOOLS/functestlib.sh

BIGTEST=$__FALSE
LITTLETEST=$__FALSE

usage(){
  echo "$0"
  echo "Check presence & operation of big.LITTLE MP Scheduler"
  echo "(c) ARM Limited 2012"
  echo ""
  echo "This script must be run as root."
  echo ""
}

#The number of cycletest threads should be atleast 1 more than the number of
#little cores to see that these little tasks in no situation get migrated to
#big core.
get_cyclictest_threads(){
  CYCLICTEST_THREADS=
  local CPUCNT=$(echo $LITTLE | wc -w)
  CPUCNT=$(($CPUCNT + 2))
  while [ "$CPUCNT" -gt 0 ]; do
    startlightloadthread
    CYCLICTEST_THREADS="$CYCLICTEST_THREADS$__RET "
    CPUCNT=$(($CPUCNT - 1))
  done
}
# The number of sysbench threads should be less than or equal to number of big
# cores so it give more probability to see them staying at big core (only in
# rare occasions if the big core is being hogged by some heavy system task,
# this big task might get pushed to the little domain considering the global
# load balancing behaviour)
get_sysbench_threads(){
  SYSBENCH_THREADS=
  local CPUCNT=$(echo $BIG | wc -w)
  while [ "$CPUCNT" -gt 0 ]; do
    startheavyloadthread
    SYSBENCH_THREADS="$SYSBENCH_THREADS$__RET "
    CPUCNT=$(($CPUCNT - 1))
  done
}

cpu_in_list(){
  FOUND=$__FALSE
  for i in $CPUS; do
    if [ "$i" == "$1" ]; then
      FOUND=$__TRUE
    fi
  done
}

check_threads(){
  local __VARNAME=$1
  local FAILED="0"
  eval $__VARNAME="'$__FALSE'"
  # we expect $CPUS to contain a string with space separated CPU numbers
  # we expect $THREADS to contain a string with space separated thread IDs
  # we return 1 if the threads in $2 are all executing on CPUs from $1
  # otherwise we return 0.
  for i in $THREADS; do
    get_thread_cpu $i
    local thread_cpu=$__RET
    echo ""Thread $i running on CPU $thread_cpu""
    if [ "$FAILED" -eq "0" ] ; then
      cpu_in_list $thread_cpu
      if [ "$FOUND" -eq "$__FALSE" ]; then
        FAILED="1"
      fi
    fi
  done
  if [ "$FAILED" -eq "0" ] ; then
    eval $__VARNAME="'$__TRUE'"
  fi
}

usage

# begin test
echo "Testing big.LITTLE MP Scheduler behaviour using simulated test load"
getbigcpulist
BIG=$__RET
echo "big CPUs are configured as $BIG"
getlittlecpulist
LITTLE=$__RET
echo "little CPUs are configured as $LITTLE"

echo "Starting tasklibrary to generate heavy CPU load"
get_sysbench_threads
echo "Starting tasklibrary to generate low CPU load"
get_cyclictest_threads

echo "Waiting for 5s"
sleep 5

echo "Checking low load threads are running on little and high load threads are running on big"
echo "High load worker threads ($SYSBENCH_THREADS):"
THREADS=$SYSBENCH_THREADS
CPUS=$BIG
check_threads "BIGTEST"
echo "Low load worker threads ($CYCLICTEST_THREADS):"
THREADS=$CYCLICTEST_THREADS
CPUS=$LITTLE
check_threads "LITTLETEST"

echo "Result of low load: $LITTLETEST"
echo "Result of high load: $BIGTEST"

for I in $CYCLICTEST_THREADS ; do
	pidkiller $I 
done
for I in $SYSBENCH_THREADS ; do
	pidkiller $I 
done

if [ $BIGTEST -eq $__TRUE ]; then
  if [ $LITTLETEST -eq $__TRUE ]; then
    echo "Test Passed"
    exit 0
  fi
fi

echo "Test Failed"
exit 1


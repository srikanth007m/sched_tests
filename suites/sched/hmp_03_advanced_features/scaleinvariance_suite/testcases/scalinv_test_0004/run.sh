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

. ../../library.sh

#
# Config options
#

# set time delay between frequency steps
STEP_DELAY=1
# set frequency of big cluster
BIG_FREQ=1200000
# set frequency of little cluster
LITTLE_FREQ=1000000
# location of tracing in filesystem
TRACEDIR="/sys/kernel/debug/tracing"

#
# test start
#

# interrogate system to determine big/little CPUs
# after this:
#  FIRST_BIG_CPU= lowest numbered big cpu
#  FIRST_LITTLE_CPU= lowest numbered little cpu
#  TARGET_LITTLE_CPU= highest numbered little cpu, where test runs
find_big_little

echo "Using cpu${FIRST_BIG_CPU} to set frequency of big cluster"
echo "Using cpu${FIRST_LITTLE_CPU} to set frequency of little cluster"
echo "Using cpu${TARGET_LITTLE_CPU} to test load scaling"

check_userspace_exists
if [ "${RET}" == "0" ] ; then
  echo "FAIL: Userspace governor not available"
  exit 1
fi

# clear existing tasks from little cpu if possible
clear_little_cpu ${TARGET_LITTLE_CPU}

# set userspace governors
userspace_governors ${FIRST_BIG_CPU} ${FIRST_LITTLE_CPU}

# start task load
echo "starting load task"
$SHELL_CMD -c 'while [ 1 ]; do I=1; done' &
TIMERLOOPPID=$!

# place task load on target CPU
move_load ${TARGET_LITTLE_CPU}

reset_tracing

# run test without frequency-invariant load scaling
echo "disabling frequency-invariant load scaling"
echo 0 > /sys/kernel/hmp/frequency_invariant_load_scale
echo ${LITTLE_FREQ} > /sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/cpufreq/scaling_setspeed
echo "starting test"
run_test
echo "done, copying trace"
cat ${TRACEDIR}/trace > ./trace_static.txt


# clean up
pidkiller $TIMERLOOPPID

# restore governors
echo "${OLD_BIG_GOVERNOR}" > /sys/devices/system/cpu/cpu${FIRST_BIG_CPU}/cpufreq/scaling_governor
echo "${OLD_LITTLE_GOVERNOR}"> /sys/devices/system/cpu/cpu${FIRST_LITTLE_CPU}/cpufreq/scaling_governor

# restore task affinity it it was modified
if [ "${AFFINITY}" == "cpusets" ] ; then
  echo "restoring tasks"
  for I in `cat /dev/cpuctl/all/tasks`; do
    echo $I > /dev/cpuctl/tasks
  done
  rmdir /dev/cpuctl/all
  rmdir /dev/cpuctl/cpu${TARGET_LITTLE_CPU}
fi

echo "Filtering Static Trace:"
build_csv trace_static.txt ${TARGET_LITTLE_CPU} > trace_static.csv.txt

echo "Checking for a result"
OVERALL_RESULT=1
echo "Evaluating Static Load Test Run"
STATIC_TEST2=0
RATIO_RANGE=10
# pass criteria 2 - the system frequency varies during the test
FIRST=1
while read TIMESTAMP FREQUENCY RATIO
do
  if [ ${FIRST} == 1 ] ; then
    FIRST=0
    TEST_FREQ=${FREQUENCY}
  fi
  if [ ${FREQUENCY} != ${TEST_FREQ} ] ; then
    STATIC_TEST2=1
  fi
done < trace_static.csv.txt
if [ ${STATIC_TEST2} == 0 ]; then
  echo "Freqinvar OFF failed frequency test due to frequency not being varied during test"
  OVERALL_RESULT=0
else
  echo "Freqinvar OFF passed frequency test"
fi

if [ $OVERALL_RESULT == 1 ] ; then
  # clean up temp files
  rm trace_static.txt
  rm trace_static.csv.txt
else
  gzip trace_static.txt
  gzip trace_static.csv.txt
fi

if [ ${OVERALL_RESULT} != 1 ] ; then
  echo "Test FAILED"
  exit 1
else
  echo "Test PASSED"
  exit 0
fi


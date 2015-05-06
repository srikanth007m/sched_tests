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

# run test using frequency-invariant load scaling
echo "enabling frequency-invariant load scaling"
echo 1 > /sys/kernel/hmp/frequency_invariant_load_scale
echo "starting test"
run_test
echo "done, copying trace"
cat ${TRACEDIR}/trace > ./trace_dynamic.txt

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

echo "Filtering Dynamic Trace:"
build_csv trace_dynamic.txt ${TARGET_LITTLE_CPU} > trace_dynamic.csv.txt

echo "Checking for a result"
OVERALL_RESULT=1
RATIO_RANGE=10
echo "Evaluating scaling Load Test Run"
# pass criteria - the tracked load (after settling) scales in line with the frequency - we have a range of +/- 1 unit because it's not always exact
DYNAMIC_TEST1=1
LAST_FREQCHANGE_TIME=0
LAST_FREQCHANGE_FREQ=0
LAST_FREQCHANGE_RATIO=0
MAX_FREQ=1000000
# check success criteria for new CPU power scale
if [ -e "/sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/topology/base_cpu_power" ] ; then
  BASEPOWER=`cat "/sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/topology/base_cpu_power"`
  MAXPOWER=`cat "/sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/topology/max_cpu_power"`
  MAX_ACHIEVABLE_RATIO=$(( (1024 * $BASEPOWER)/MAXPOWER ))
else
  # using older frequency-invariance only
  MAX_ACHIEVABLE_RATIO=1024
fi

echo "Maximum achievable ratio at 100% freq for this CPU is $MAX_ACHIEVABLE_RATIO"

while read TIMESTAMP FREQUENCY RATIO
do
  if [ $LAST_FREQCHANGE_FREQ != $FREQUENCY ] ; then
    FREQ=$LAST_FREQCHANGE_FREQ
    R=$LAST_FREQCHANGE_RATIO
    FREQ_RATIO=$(( (1024*($FREQ/1000))/($MAX_FREQ/1000) ))
    TARGET=$(( (($MAX_ACHIEVABLE_RATIO * $FREQ_RATIO) / 1024) - 1 )) # -1 to account for persistent rounding
    echo "At $LAST_FREQCHANGE_TIME, ratio is $LAST_FREQCHANGE_RATIO and calculated target is $TARGET"
    LOWEST_RATIO=$((${TARGET}-${RATIO_RANGE}))
    HIGHEST_RATIO=$((${TARGET}+${RATIO_RANGE}))
    if (( ( $LAST_FREQCHANGE_RATIO < $LOWEST_RATIO ) )) ; then
      DYNAMIC_TEST1=0
    fi
    if (( ( $LAST_FREQCHANGE_RATIO > $HIGHEST_RATIO ) )); then
      DYNAMIC_TEST1=0
    fi
  fi
  LAST_FREQCHANGE_FREQ="$FREQUENCY"
  LAST_FREQCHANGE_TIME="$TIMESTAMP"
  LAST_FREQCHANGE_RATIO="$RATIO"
done < trace_dynamic.csv.txt

if [ ${DYNAMIC_TEST1} != 1 ] ; then
  echo "Freqinvar ON failed frequency scaling test by not settling in line with frequency changes"
  OVERALL_RESULT=0
else
  echo "Freqinvar ON passed frequency scaling test"
fi

if [ $OVERALL_RESULT == 1 ] ; then
  # clean up temp files
  rm trace_dynamic.txt
  rm trace_dynamic.csv.txt
else
  gzip trace_dynamic.txt
  gzip trace_dynamic.csv.txt
fi


if [ ${OVERALL_RESULT} != 1 ] ; then
  echo "Test FAILED"
  exit 1
else
  echo "Test PASSED"
  exit 0
fi


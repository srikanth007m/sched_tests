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

# Check the behavior of Small Task Packing
#
# Run as root to allow task killing after testing.
#

. ../../taskpackinglib.sh

BIGTEST=$__FALSE
LITTLETEST=$__FALSE

usage(){
  echo "$0"
  echo "Check the behavior of Small Task Packing"
  echo "(c) ARM Limited 2014"
  echo ""
  echo "This script must be run as root."
  echo ""
}

get_cyclictest_threads(){
  CYCLICTEST_THREADS=
  for i in $LITTLE + 2; do
    startlightloadthread
    CYCLICTEST_THREADS="$CYCLICTEST_THREADS$__RET "
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
  local SMALL_TASK_CPU=
  eval $__VARNAME="'$__FALSE'"
  # we expect $CPUS to contain a string with space separated CPU numbers
  # we expect $THREADS to contain a string with space separated thread IDs
  # we return 1 if the threads in $2 are all executing on CPUs from $1
  # otherwise we return 0.
  for i in $THREADS; do
    get_thread_cpu $i
    local thread_cpu=$__RET
    echo ""Thread $i running on CPU $thread_cpu""
    if [ "$FAILED" -eq "0" ]; then
      cpu_in_list $thread_cpu
      if [ "$FOUND" -eq "$__FALSE" ] || { [ -n "$SMALL_TASK_CPU" ] && \
                                [ "$SMALL_TASK_CPU" != "$thread_cpu" ];}; then
        FAILED="1"
      fi
      SMALL_TASK_CPU="$thread_cpu"
    fi
  done
  if [ "$FAILED" -eq "0" ] ; then
    eval $__VARNAME="'$__TRUE'"
  fi
}

usage

# begin test
probe_for_small_task_packing

# Enable small task packing and set the packing limit to 1024
enable_small_task_packing
set_packing_limit 1024

echo "Testing big.LITTLE MP small task packing behaviour using simulated test load"
getlittlecpulist
LITTLE=$__RET
echo "little CPUs are configured as $LITTLE"

echo "Starting tasklibrary to generate low CPU load"
get_cyclictest_threads

echo "Waiting for 5s"
sleep 5

echo "Checking low load threads are packed on a single little cpu"
echo "Low load worker threads ($CYCLICTEST_THREADS):"
THREADS=$CYCLICTEST_THREADS
CPUS=$LITTLE
check_threads "LITTLETEST"

echo "Result of low load: $LITTLETEST"

for I in $CYCLICTEST_THREADS ; do
  if [ $ANDROID -eq 0 ]; then
    disown $I
  fi
  kill -9 $I > /dev/null 2>&1
done

restore_packing_limit
restore_small_task_packing_state

if [ $LITTLETEST -eq $__TRUE ]; then
  echo "Test Passed"
  exit 0
fi

echo "Test Failed"
exit 1

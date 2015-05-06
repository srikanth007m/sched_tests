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

#import test functions library
source $TOOLS/functestlib.sh

# sched_domain functional testing of big.LITTLE MP Scheduler

# FT0011
# Assertion: The little CPUs calculated by the test framework are A7s/A53s
TESTNAME="FT0011"
RESULT="1"
getlittlecpulist
HMPSLOWCPUS=$__RET
echo "The test framework thinks that the slow CPUs are $__RET"
getcpuarray
CPUSTRING=$__RET
LITTLECPUS=""
echo "We have the following CPUs available: $CPUSTRING"
for CPU in $CPUSTRING; do
  getcputype $CPU
  if [ "x$__RET" == "x$CONFIG_TARGET_LITTLE_CPUPART" ] ; then
    LITTLECPUS="$LITTLECPUS$CPU "
  fi
done

echo "CPUS $LITTLECPUS are the A7/A53 CPUs in this system"
cpustringtobitfield $HMPSLOWCPUS
HMPBITS=$__RET
cpustringtobitfield $LITTLECPUS
LITTLEBITS=$__RET

TEST=$(( (0x$HMPBITS) - (0x$LITTLEBITS) ))
if [ $TEST -ne 0 ] ; then
  RESULT=0
fi

if [ "$RESULT" != "1" ] ; then
  echo "$TESTNAME Failed"
  exit 1
fi

echo "$TESTNAME Passed"
exit 0


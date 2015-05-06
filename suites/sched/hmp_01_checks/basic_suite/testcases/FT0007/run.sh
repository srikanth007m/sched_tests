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

# FT0007
# Assertion: The big CPUs calculated by the test framework are A15s/A57s
TESTNAME="FT0007"
RESULT="1"
getbigcpulist
HMPFASTCPUS=$__RET
echo "The test framework thinks that the fast CPUs are $__RET"
getcpuarray
CPUSTRING=$__RET
BIGCPUS=""
echo "We have the following CPUs available: $CPUSTRING"
for CPU in $CPUSTRING; do
  getcputype $CPU
  if [ "x$__RET" == "x$CONFIG_TARGET_BIG_CPUPART" ] ; then
    BIGCPUS="$BIGCPUS$CPU "
  fi
done

echo "CPUS $BIGCPUS are the A15/A57 CPUs in this system"
cpustringtobitfield $HMPFASTCPUS
HMPBITS=$__RET
cpustringtobitfield $BIGCPUS
BIGBITS=$__RET

TEST=$(( (0x$HMPBITS) - (0x$BIGBITS) ))
if [ $TEST -ne 0 ] ; then
  RESULT=0
fi

if [ "$RESULT" != "1" ] ; then
  echo "$TESTNAME Failed"
  exit 1
fi

echo "$TESTNAME Passed"
exit 0


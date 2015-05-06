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

# FT0008
# Assertion: All big CPUs take part in a sched domain populated entirely by other big CPUs
TESTNAME="FT0008"
RESULT="1"
getbigcpulist
HMPFASTCPUS=$__RET
echo "The test framework thinks that the fast CPUs are $__RET"
cpustringtobitfield $HMPFASTCPUS
HMPBITS=$__RET
for CPU in $HMPFASTCPUS; do
  getpackagescheddomainbitfield "$CPU"
  echo "cpu$CPU sched_domain at MC level covers CPUs 0x${__RET}."
  TEST=$(( (0x${__RET}) - (0x${HMPBITS}) ))
  if [ $TEST -ne 0 ] ; then
    RESULT=0
    break
  fi
done


if [ "$RESULT" != "1" ] ; then
  echo "$TESTNAME Failed"
  exit 1
fi

echo "$TESTNAME Passed"
exit 0


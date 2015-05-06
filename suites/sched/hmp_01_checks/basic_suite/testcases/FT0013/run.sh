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

# FT0013
# Assertion: The scheduler is not printing CPU indication in the boot log.
# NOTE: HMP Scheduler used to print this info in the boot log, but does
#       not do this any longer.

TESTNAME="FT0013"
RESULT="0"

#check for fast CPU
getbootlogfastcpulist
HMPFASTCPUS=$__RET
if [ ${#HMPFASTCPUS} -eq 0 ] ; then
  echo "Failed to find fast CPU indication in boot log."
  RESULT_fast="1"
else
  echo "The scheduler said in the boot log that the fast CPUs were $__RET"
  RESULT_fast="0"
fi

#check for slow CPU
getbootlogslowcpulist
HMPSLOWCPUS=$__RET
if [ ${#HMPFASTCPUS} -eq 0 ] ; then
  echo "Failed to find slow CPU indication in boot log."
  RESULT_slow="1"
else
  echo "The scheduler said in the boot log that the slow CPUs were $__RET"
  RESULT_slow="0"
fi

# combine the results
if [ ${RESULT_slow} == "1" ] ; then
  if [ ${RESULT_fast} == "1" ] ; then
    RESULT="1"
  fi
fi

if [ "$RESULT" != "1" ] ; then
  echo "$TESTNAME Failed"
  exit 1
fi

echo "$TESTNAME Passed"
exit 0


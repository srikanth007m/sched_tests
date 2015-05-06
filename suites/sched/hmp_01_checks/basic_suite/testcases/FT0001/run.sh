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

# FT0001
# Assertion: The scheduler has a CPU-level domain covering all CPUs
TESTNAME="FT0001"
RESULT="1"
getcpuarray
CPUSTRING=$__RET
echo "We have the following CPUs available: $CPUSTRING"
for CPU in $CPUSTRING; do
  getcpuscheddomainbitfield "$CPU"
  TMP="$__RET"
  echo "cpu$CPU sched_domain at CPU level covers CPUs 0x$TMP"
  for BIT in $CPUSTRING; do
    isbitsetinbitfield "$BIT" "$TMP"
    if [ "$?" -eq "$__FALSE" ] ; then
      RESULT="0"
      break 2
    fi
  done
done

if [ "$RESULT" != "1" ] ; then
  echo ""$TESTNAME Failed""
  exit 1
fi

echo ""$TESTNAME Passed""
exit 0


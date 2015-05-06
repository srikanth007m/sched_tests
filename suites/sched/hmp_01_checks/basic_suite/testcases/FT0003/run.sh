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

# FT0003
# Assertion: The scheduler has an MC-level domain covering all CPU siblings
TESTNAME="FT0003"
RESULT="1"
getcpuarray
CPUSTRING=$__RET
echo "We have the following CPUs available: $CPUSTRING"
for CPU in $CPUSTRING; do
  getpackagescheddomainbitfield "$CPU"
  TMP="$__RET"
  getcpusiblings "$CPU"
  CPUSIBS=$__RET
  echo "cpu$CPU sched_domain at MC level covers CPUs 0x${TMP}. cpu$CPU siblings are 0x${CPUSIBS}"
  hextodec $TMP
  TMP=$__RET
  hextodec $CPUSIBS
  if [ "$__RET" -ne "$TMP" ] ; then
    RESULT="0"
    break
  fi
done

if [ "$RESULT" != "1" ] ; then
  echo "$TESTNAME Failed"
  exit 1
fi

echo "$TESTNAME Passed"
exit 0


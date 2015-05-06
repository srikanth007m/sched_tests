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
. $TOOLS/functestlib.sh

# test library is working
functestlibdoc

echo ""
echo "Testing individual functions"
getcpuarray
CPUSTRING=$__RET
echo "getcpuarray reports CPUs $CPUSTRING"
TESTNUM="12345678"
dectohex "$TESTNUM"
HEXNUM=$__RET
echo "dectohex converted $TESTNUM to $HEXNUM"
hextodec "$HEXNUM"
echo "hextodec converted $HEXNUM to $__RET"
cpustringtobitfield $CPUSTRING
CPUBITFIELD=$__RET
echo "cpustringtobitfield converted $CPUSTRING to $CPUBITFIELD"
echo "testing if cpu 0 is in the bitfield."
isbitsetinbitfield 0 $CPUBITFIELD
RESULT=$?
if [ "$RESULT" -eq "$__TRUE" ] ; then
  echo " YES"
else
  echo " NO"
fi
echo "testing if cpu 32 is in the bitfield. (If you have >32 CPUs this will not work)"
isbitsetinbitfield 32 $CPUBITFIELD
RESULT=$?
if [ "$RESULT" -eq "$__TRUE" ] ; then
  echo " YES"
else
  echo " NO"
fi
echo "getting getcpusiblings for CPU0"
getcpusiblings 0
CPUSIBS=$__RET
echo "checking if CPU1 is a core sibling of CPU0"
isbitsetinbitfield 1 $CPUSIBS
RESULT=$?
if [ "$RESULT" -eq "$__TRUE" ] ; then
  echo " YES"
else
  echo " NO"
fi
echo "checking if CPU2 is a core sibling of CPU0"
isbitsetinbitfield 2 $CPUSIBS
RESULT=$?
if [ "$RESULT" -eq "$__TRUE" ] ; then
  echo " YES"
else
  echo " NO"
fi
echo "Checking availability of domain flags"
domainflagssupported
DOMAIN_FLAGS_SUPPORTED=$?
echo "domainflagssupported returned: $DOMAIN_FLAGS_SUPPORTED"
if [ "$DOMAIN_FLAGS_SUPPORTED" -eq "$__TRUE" ] ; then
  echo " Domain flags supported"
else
  echo " No domain flags supported"
fi
echo "Get CPU Domain bitfield for CPU0"
getcpuscheddomainbitfield 0
TMP="$__RET"
echo "Get CPU Domain flags for CPU0"
getcpuscheddomainflags 0
TMP2="$__RET"
echo "Get CPU Domain flags printable string for CPU0"
getprintableschedflags "$TMP2"
echo "CPU Domain bitfield for CPU0 is $TMP. Flags are $TMP2 $__RET"
echo "Get Package Domain bitfield for CPU0"
getpackagescheddomainbitfield 0
TMP="$__RET"
echo "Get Package Domain flags for CPU0"
getpackagescheddomainflags 0
TMP2="$__RET"
echo "Get Package Domain flags printable string for CPU0"
getprintableschedflags "$TMP2"
echo "Package Domain (MT/MC) bitfield for CPU0 is $TMP. Flags are $TMP2 $__RET"
echo "Get big CPU list"
getbigcpulist
BIGCPUS="$__RET"
echo "Get little CPU list"
getlittlecpulist
LITTLECPUS="$__RET"
echo "Big CPUs are $BIGCPUS and little CPUs are $LITTLECPUS"

RANGE="0-1,3,5-10,6"
cpurangetolist $RANGE
echo "converted CPU range string $RANGE to list $__RET"


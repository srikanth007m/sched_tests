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

#FT0000
#Assertion: The scheduler has HMP support
TESTNAME="FT0000"
RESULT="1"
CONFIG="/proc/config.gz"
HMPFILE="hmp.config"
HMP="CONFIG_SCHED_HMP=y"
SYSDIR="/sys/kernel/hmp"

if [ ! -s $CONFIG ]; then
   echo "Config Not found, Trying sysfs"
   if [ ! -d $SYSFILE ]; then
      RESULT="1"
   else
      RESULT="0"
   fi
else
   zcat $CONFIG | grep $HMP > $HMPFILE
   if [ ! -s $HMPFILE ]; then
      RESULT="1"
   else
      cat $HMPFILE
      RESULT="0"
   fi
   rm -f $HMPFILE
fi
if [ "$RESULT" != "1" ]; then
  echo $TESTNAME "Test Passed"
  exit 0
else
  echo $TESTNAME "Test Failed"
  exit 1
fi

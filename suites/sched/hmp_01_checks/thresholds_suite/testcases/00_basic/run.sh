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

# Migration thresolds sysfs attributes
UTFILE=/sys/kernel/hmp/up_threshold
DTFILE=/sys/kernel/hmp/down_threshold

echo
echo "Migration thresholds configuration:"
UP_THRESHOLD=`cat $UTFILE`
DW_THRESHOLD=`cat $DTFILE`
echo "  up_threshold   : $UP_THRESHOLD"
echo "  down_threshold : $DW_THRESHOLD"

RESULT="0"

echo
echo "Checking for thresholds boundaries..."
if [ $UP_THRESHOLD -ge 1023 ]; then
  echo "  ERROR: Up migration threshold saturated!"
  echo "         Must be < 1023"
  RESULT=1
fi
if [ $DW_THRESHOLD -le 0 ]; then
  echo "  ERROR: Down migration threshold saturated!"
  echo "         Must be > 0"
  RESULT=1
fi

echo
echo "Checking for thresholds invertion..."
if [ $UP_THRESHOLD -le $DW_THRESHOLD ]; then
  echo "  ERROR: Migration thresholds invertion!"
  echo "         up_migration must be > down_migration"
  RESULT=1
fi

echo
if [ $RESULT = 0 ] ; then
  echo "SUCCESS"
else
  echo "FAILED"
fi
exit $RESULT


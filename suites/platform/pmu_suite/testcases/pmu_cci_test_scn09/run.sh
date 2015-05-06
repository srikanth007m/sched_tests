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
. ../../functestlib.sh

TESTNAME="MP_CCI_TEST_SCN09"

PROG="$TOOLS/cache"

MASK=$(build_cpu_mask $__PART_A7)

CFG_VALS="0xFF 0x83 0x92 0x63 0x72"
CFG_NAMES="cycles a7_cacheable_read a7_evict_write a15_cacheable_read a15_evict_write"

CFG_STR=$( create_cfg_str "$CFG_NAMES" "$CFG_VALS" "$__PMU_CCI" "cci")

__SCALES=(1000 5000 10000)
for __I in ${__SCALES[@]}; do
    COUNT=$(echo "100 * 10 ^ $__I" | $TOOLS/bc)
    PARAM="-o $__I.dat -x, -a -C 0 -e $CFG_STR $__TASKSET $MASK $PROG -l $__I"
    echo "$__PERF stat $PARAM"
    $__PERF stat $PARAM
done

test_event "cci_a7_cacheable_read" "$(stringify ${__SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "cci_a7_evict_write" "$(stringify ${__SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"

if [ $__RESULT -ne 1 ]; then
    echo "$TESTNAME Failed"
    exit 1
fi
echo "$TESTNAME Passed"

exit 0

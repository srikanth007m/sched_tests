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

TESTNAME="MP_PMU_TEST_SCN05"

PROG="$TOOLS/cache"

CFG_VALS_A7="0xFF 0x08 0x04 0x03 0x0C 0x10"
CFG_NAMES="cpu_cycles instructions cache_references cache_misses branch_instructions branch_misses"

CFG_STR=$(create_cfg_str "$CFG_NAMES" "$CFG_VALS_A7" "$__PMU_A7" "a7")

SCALES=(1000 5000 10000)

MASK=$(build_cpu_mask $__PART_A7)

for I in ${SCALES[@]}; do
	PARAM="-x, -o $I.dat -g -e $CFG_STR $__TASKSET $MASK $PROG -l $I"
	echo "$__PERF stat $PARAM"
	$__PERF stat $PARAM
done

test_event "a7_instructions" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "a7_cache_references" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "a7_cache_misses" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event_not_supported "a7_branch_misses" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"

if [ $__RESULT -ne 1 ]; then
	echo "$TESTNAME Failed"
	exit 1
fi
echo "$TESTNAME Passed"

exit 0


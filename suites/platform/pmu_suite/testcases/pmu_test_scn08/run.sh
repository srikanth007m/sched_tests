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

TESTNAME="MP_PMU_TEST_SCN08"

PROG="$TOOLS/branch"

CFG_VALS_A7="0x0C 0x10 0x1D"
CFG_VALS_A15="0x76 0x10 0x1D"
CFG_NAMES="branch_instructions branch_misses bus_cycles"

CFG_STR=$(create_cfg_str "$CFG_NAMES" "$CFG_VALS_A7" "$__PMU_A7" "a7" "$CFG_VALS_A15" "$__PMU_A15" "a15")

disable_cpu_idle

SCALES=(1 4 16)

MASK=$(build_cpu_mask $__PART_A7)

for I in ${SCALES[@]}; do
	PARAM="-x, -o $I.dat -g -e $CFG_STR $__TASKSET $MASK $PROG -l $I"
	echo "$__PERF stat $PARAM"
	$__PERF stat $PARAM
done

test_event "a7_branch_instructions" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "a7_branch_misses" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"

MASK=$(build_cpu_mask $__PART_A15)

for I in ${SCALES[@]}; do
	PARAM="-x, -o $I.dat -g -e $CFG_STR $__TASKSET $MASK $PROG -l $I"
	echo "$__PERF stat $PARAM"
	$__PERF stat $PARAM
done

test_event "a15_branch_instructions" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "a15_branch_misses" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"

enable_cpu_idle

if [ $__RESULT -ne 1 ]; then
	echo "$TESTNAME Failed"
	exit 1
fi
echo "$TESTNAME Passed"

exit 0


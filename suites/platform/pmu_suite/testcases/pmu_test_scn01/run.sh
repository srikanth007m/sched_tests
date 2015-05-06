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

# Current limitations:

TESTNAME="MP_PMU_TEST_SCN01"

PROG="$TOOLS/cycles"

CFG_VALS_A7="0xFF 0x08"
CFG_VALS_A15="0xFF 0x08"
CFG_NAMES="cpu_cycles instructions"

CFG_STR=$(create_cfg_str "$CFG_NAMES" "$CFG_VALS_A7" "$__PMU_A7" "a7" "$CFG_VALS_A15" "$__PMU_A15" "a15" "u")

SCALES[0]=$(echo "256*1024*1024" | $TOOLS/bc)
SCALES[1]=$(echo "512*1024*1024" | $TOOLS/bc)
SCALES[2]=$(echo "1024*1024*1024" | $TOOLS/bc)

MASK=$(build_cpu_mask $__PART_A7)

for I in ${SCALES[@]}; do
	PARAM="-x, -o $I.dat -e $CFG_STR $__TASKSET $MASK $PROG -l $I"
	echo "$__PERF stat $PARAM"
	$__PERF stat $PARAM
done

test_event "a7_cpu_cycles" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "a7_instructions" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"

MASK=$(build_cpu_mask $__PART_A15)

for I in ${SCALES[@]}; do
	PARAM="-x, -o $I.dat -e $CFG_STR $__TASKSET $MASK $PROG -l $I"
	echo "$__PERF stat $PARAM"
	$__PERF stat $PARAM
done

test_event "a15_cpu_cycles" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"
test_event "a15_instructions" "$(stringify ${SCALES[@]})"
[ "$?" -eq $__FALSE ] && __RESULT="0"

if [ $__RESULT -ne 1 ]; then
	echo "$TESTNAME Failed"
	exit 1
fi
echo "$TESTNAME Passed"

exit 0


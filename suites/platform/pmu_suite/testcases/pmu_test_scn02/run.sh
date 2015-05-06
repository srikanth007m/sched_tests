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

TESTNAME="MP_PMU_TEST_SCN02"

PROG="$TOOLS/cache"

CFG_STR="cache-references -e cache-misses -e bus-cycles"

SCALES=(1000 5000 10000)

CPUS=(0 1 2 3 4)

for CPU in ${CPUS[@]}; do
	(( MASK = 1 << $CPU))
	MASK=$(printf "0x%X" $MASK)
	for I in ${SCALES[@]}; do
		PARAM="-o $I.dat -x, -a -C $CPU -e $CFG_STR $__TASKSET $MASK $PROG -l $I"
		echo "$__PERF stat $PARAM"
		$__PERF stat $PARAM
	done

	test_event "cache-references" "$(stringify ${SCALES[@]})"
	[ "$?" -eq $__FALSE ] && __RESULT="0"
	test_event "cache-misses" "$(stringify ${SCALES[@]})"
	[ "$?" -eq $__FALSE ] && __RESULT="0"
done

if [ $__RESULT -ne 1 ]; then
	echo "$TESTNAME Failed"
	exit 1
fi
echo "$TESTNAME Passed"

exit 0


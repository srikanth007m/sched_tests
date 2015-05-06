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

TESTNAME="HWBP_TEST_01"

PROG="$TOOLS/breakpoint_test"
TASKSET="$TOOLS/taskset"

CPUS=(0 1 2 3 4)

RESULT=1

for CPU in ${CPUS[@]}; do
	(( MASK = 1 << $CPU))
	MASK=$(printf "0x%X" $MASK)
	echo "Cpu mask = $MASK"
	$TASKSET $MASK $PROG
	[ $? -eq 0 ] || RESULT=0
done

if [ $RESULT -ne 1 ]; then
	echo "$TESTNAME Failed"
	exit 1
fi
echo "$TESTNAME Passed"

exit 0


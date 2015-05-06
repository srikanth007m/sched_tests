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

. $TOOLS/basics.sh

echo "This starts with light task(s) on little domain however"
echo "the computed load increases due to load pattern (being in runQ)."
echo "Hence the migration criteria is met it migrates to big domain."
echo "The task gets the CPU immediately."


ftrace_start
load_generator "0,L.$LITTLE_THRESHOLD.$CUTOFF_PRIORITY_GT:2000,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_GT:2999,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_GT:3000,end" START_LITTLE
PID=$RESULT
wait $PID
ftrace_stop

#The following variables are used by ftrace_check
EXPECTED_CHANGE_TIME_MS_MIN=1700
EXPECTED_CHANGE_TIME_MS_MAX=2200
START_LITTLE=$PID
START_LITTLE_PRIORITY=-1
START_BIG=
START_BIG_PRIORITY=
END_LITTLE=
END_LITTLE_PRIORITY=
END_BIG=$PID
END_BIG_PRIORITY=-1
ftrace_check $BIG_LITTLE_SWITCH_SO

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT








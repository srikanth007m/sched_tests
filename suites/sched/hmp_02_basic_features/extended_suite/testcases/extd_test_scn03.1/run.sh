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
echo "This scenario assumes the heavy task is already in big"
echo "domain however its computed load is decreasing due to"
echo "the load pattern (due to idling). The task stays in"
echo "big domain since the task load has not yet crossed below"
echo "the down-threshold. The runnable task gets CPU immediately. "

ftrace_start
load_generator "0,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_GT:3000,B.$NODOWN_THRESHOLD.$CUTOFF_PRIORITY_GT:7000,end" START_FAST
PID=$RESULT
wait $PID
ftrace_stop

#The following variables are used by ftrace_check
EXPECTED_CHANGE_TIME_MS_MIN=
EXPECTED_CHANGE_TIME_MS_MAX=
START_LITTLE=
START_LITTLE_PRIORITY=
START_BIG=$PID
START_BIG_PRIORITY=-1
END_LITTLE=
END_LITTLE_PRIORITY=
END_BIG=$PID
END_BIG_PRIORITY=-1
EXPECTED_TIME_IN_END_STATE_MS=5000

ftrace_check $BIG_LITTLE_SWITCH_SO

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT








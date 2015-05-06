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
echo "This starts with a heavy task on big domain without any specific CPU affinity"
echo "defined. However the task’s CPU affinity is modified mid way through execution"
echo "to little domain CPU forcing a migration to Little domain. "
echo "The task gets the CPU immediately."

ftrace_start
load_generator "0,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_GT:4000,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_GT:7000,end" START_FAST
PID=$RESULT
sleep 3
taskset_cpuslow $PID
wait $PID
ftrace_stop

#The following variables are used by ftrace_check
EXPECTED_CHANGE_TIME_MS_MIN=2800
EXPECTED_CHANGE_TIME_MS_MAX=3500
START_LITTLE=
START_LITTLE_PRIORITY=
START_BIG=$PID
START_BIG_PRIORITY=-1
END_LITTLE=$PID
END_LITTLE_PRIORITY=-1
END_BIG=
END_BIG_PRIORITY=
ftrace_check $BIG_LITTLE_SWITCH_SO

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT








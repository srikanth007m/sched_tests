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
echo "This starts with a heavy task however the task priority"
echo "is modified mid way through execution below cutoff. Since the"
echo "big domain is oversubscribed, the task stays on little domain"

ftrace_start
hog_cpu_fast
load_generator "0,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_GT:2000,B.$BIG_THRESHOLD.$CUTOFF_PRIORITY_LT:3000,end" START_SLOW
PID=$RESULT
wait $PID
ftrace_stop
unhog_cpu

#The following variables are used by ftrace_check
EXPECTED_CHANGE_TIME_MS_MIN=1800
EXPECTED_CHANGE_TIME_MS_MAX=2200
EXPECTED_TIME_IN_END_STATE_MS=600
START_LITTLE=$PID
START_LITTLE_PRIORITY=$CUTOFF_PRIORITY_GT
START_BIG=
START_BIG_PRIORITY=
END_LITTLE=$PID
END_LITTLE_PRIORITY=$CUTOFF_PRIORITY_LT
END_BIG=
END_BIG_PRIORITY=
ftrace_check $BIG_LITTLE_SWITCH_SO

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT

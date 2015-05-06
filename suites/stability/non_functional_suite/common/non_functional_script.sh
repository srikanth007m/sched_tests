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

#import platform
. $TOOLS/platform.sh

BUNCH_OF_TASKS=$TOOLS/bunch_of_tasks
TASKSET=$TOOLS/affinity_tools

TIME_PER_PROCESS=20
TIME_RUNNING=0.0009
TIME_SLEEPING=0.0001

#time each task must run for.
TEST_TIME_TASK=240
CPU_ALL=
i=0
SPACE=
CPU_NB=0
CPU_FAST=
CPU_SLOW=
CPU_ALL=
#ARM
IMPLEMENTER=0x41
#LITTLE CPU part
PART_SLOW=`echo $CONFIG_TARGET_LITTLE_CPUPART`
#big CPU part
PART_FAST=`echo $CONFIG_TARGET_BIG_CPUPART`
commaslow=
commafast=
comma=
cpu=0
end=0
while [ $end == 0 ] ; do
	end=1
        $TASKSET -part $cpu,$IMPLEMENTER,$PART_SLOW >/dev/null
        if [ $? == 0 ] ; then
                CPU_SLOW=$CPU_SLOW$commaslow$cpu
		CPU_ALL="$CPU_ALL$comma$cpu"
                commaslow=" "
		comma=" "
		end=0
        fi
        $TASKSET -part $cpu,$IMPLEMENTER,$PART_FAST >/dev/null
        if [ $? == 0 ] ; then
                CPU_FAST=$CPU_FAST$commafast$cpu
		CPU_ALL="$CPU_ALL$comma$cpu"
                commafast=" "
		comma=" "
		end=0
        fi
	let cpu=$cpu+1
done
echo "Fast CPU $CPU_FAST Slow CPU $CPU_SLOW"

#General rule; function with return values, put
# the return value in RESULT

# return the current time
function get_time()
{
	RESULT="`date +%s`"
}

# get_field $1 $2 $3 $4 ...
# return $[$1+2]
function get_field()
{
	f2i=$1
	while [ $f2i -ge 1 ] ; do
		shift
		let f2i=$f2i-1
	done
	RETURN=$2
}


# save_proc "FILE"
function save_proc()
{
	#Preserve RESULT
	f3RESULT=$RESULT
	get_time
	f3b=$RESULT
	for f3i in /proc/*/stat ; do
		f3a=`cat $f3i 2>/dev/null `
		echo "$f3i $f3b $f3a" >> $1
	done
	cat /proc/interrupts | while read f3i ; do
		echo "/proc/interrupts $f3b $f3i" >> $1
	done
	RESULT=$f3RESULT
}

#create_task "NICE0 NICE1 NICE2..." NUMBER_OF_REPEAT
#RESULT=PID_PROCESS
function create_task()
{
	f4all=
	f4i=1
	while [ $f4i -le $2 ] ; do
		f4all="$f4all $1"
		let f4i=$f4i+1
	done

	$BUNCH_OF_TASKS NO_BALANCE_CHECK $TIME_PER_PROCESS $TIME_RUNNING $TIME_SLEEPING $f4all &
	RESULT=$!
}

#create_task "NICE0 NICE1 NICE2..." NUMBER_OF_REPEAT
#RESULT=PID_PROCESS
function create_task_balance_check()
{
	f4all=
	f4i=1
	while [ $f4i -le $2 ] ; do
		f4all="$f4all $1"
		let f4i=$f4i+1
	done

	$BUNCH_OF_TASKS $TIME_PER_PROCESS $TIME_RUNNING $TIME_SLEEPING $f4all &
	RESULT=$!
}

#check_running PID0
#RESULT=
#   0 running
#   1 ended succesffully
#   2 ended with error
function check_running()
{
	if [ -f /proc/$1/stat ] ; then
		RESULT=0
	else
		wait $1
		if [ $? -le 0 ] ; then
			RESULT=1
		else
			RESULT=2
		fi
	fi
}


#hot_unplug "CPU0 CPU1 CPU2 ..."
# hot unplug cpu
# RESULT = 0 ok
#	 = 1 failed

function hot_unplug()
{
	RESULT=0
	for f5i in $1 ; do
		echo 0 > /sys/devices/system/cpu/cpu$f5i/online
		f5temp=`cat /sys/devices/system/cpu/cpu$f5i/online`
		if [ "$f5temp" == 1 ] ; then
			RESULT=1
		fi
	done
}

#hot_plug "CPU0 CPU1 CPU2 ..."
# hot plug cpu
# RESULT = 0 ok
#	 = 1 failed
function hot_plug()
{
	RESULT=0
	for f5i in $1 ; do
		echo 1 > /sys/devices/system/cpu/cpu$f5i/online
		f5temp=`cat /sys/devices/system/cpu/cpu$f5i/online`
		if [ "$f5temp" == 0 ] ; then
			RESULT=1
		fi
	done
}

# save_cpuidle "FILE"
# save /sys/devices/system/cpu/cpu*/cpuidle/*
function save_cpuidle()
{
	#preserve RESULT
	f6RESULT=$RESULT
	get_time
	f6b=$RESULT
	for f6i in /sys/devices/system/cpu/cpu*/cpuidle/*/* ; do
		f6a=`cat $f6i 2>/dev/null `
		echo "$f6i $f6b $f6a" >> $1
	done
	RESULT=$f6RESULT
}


# get_cpuidle_usage "C1/C2/C3" CPU_INDEX
function get_cpuidle_usage()
{
	RESULT=0
	for f7i in /sys/devices/system/cpu/cpu$2/cpuidle/*/ ; do
		if [ "`cat $f7i/name`" == "$1" ] ; then
			RESULT="`cat $f7i/usage`"
		fi
	done
}




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

. ../../non_functional_script.sh
echo "Idling for long period"

function idle_test()
{
	save_proc proc.dat

	for i in 1 2 3 4 ; do
	# sleep to let cpuidle menu understand we always sleep for a while
		sleep 1
	done

	get_cpuidle_usage C2 0
	CPU0_C2=$RESULT
	get_cpuidle_usage C1 0
	CPU0_C1=$RESULT

	CPU0_C2_START=$CPU0_C2
	CPU0_C1_START=$CPU0_C1

	i=0
	while [ $i -le 20 ] ; do
		prev_i=$i
		let i=$i+1
		save_cpuidle cpuidle.log
		sleep 1

		CPU0_C2_OLD=$CPU0_C2
		CPU0_C1_OLD=$CPU0_C1
		get_cpuidle_usage C2 0
		CPU0_C2=$RESULT
		get_cpuidle_usage C1 0
		CPU0_C1=$RESULT

		if [ "$CPU0_C2" == "$CPU0_C2_OLD" ] ; then
			if [ "$CPU0_C1" == "$CPU0_C1_OLD" ] ; then
				echo "Not entered in C2 or C1 in CPU0, during the last seconds - $prev_i s to $i s-."
				echo "cpuidle might be broken."
			fi
		fi
	done

	save_proc proc.dat

	let CPU0_C1=$CPU0_C1-$CPU0_C1_START
	let CPU0_C2=$CPU0_C2-$CPU0_C2_START

	if [ "$CPU0_C2" == "0" ] ; then
		if [ "$CPU0_C1" == "0" ] ; then
			echo "Not entered in C2 or C1 in CPU0, during the test."
			echo "cpuidle might be broken."
		fi
	fi
	echo "CPU0 was $CPU0_C1 times in state C1 and $CPU0_C2 times in state C2"
}


echo "Repeated entry of idle scenario"
# Stop all android services
if [ $ANDROID -eq 1 ]; then
	stop
fi
get_time
START=$RESULT
#8 Hours
let END=$START+28800
loop=0
while [ $RESULT -le $END ] ; do
	echo
	echo "------------- IDLE START "
	echo
	idle_test
	echo
	echo "------------- IDLE STOP"
	let loop=$loop+1
	get_time
	echo
	echo "--- Start $START -- Now $RESULT -- End $END --- loop $loop ---"
done

#Never fail
echo SUCCESS
exit 0

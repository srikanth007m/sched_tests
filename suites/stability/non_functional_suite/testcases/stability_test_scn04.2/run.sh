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
echo "Hot unplug all big cores when large pool of tasks are being active"
# Stop all android services
if [ $ANDROID -eq 1 ]; then
	stop
fi

create_task 0 200
PID=$RESULT

#Make sure everythings is started
sleep 4

echo "Unplug fast cpu"
hot_unplug "$CPU_FAST"

RESULT=0
while [ $RESULT -le 0 ] ; do
	sleep 2
	save_proc proc.dat
	check_running $PID
done
save_proc proc.dat
hot_plug "$CPU_FAST"

if [ $RESULT -le 1 ] ; then
	echo SUCCESS
	exit 0
fi
echo FAILED
exit 1

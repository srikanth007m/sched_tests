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
echo "Large task pool for HMP migration for at least 10 Hours"
# Stop all android services
if [ $ANDROID -eq 1 ]; then
	stop
fi


#Start + 10hours
get_time
START=$RESULT
let END=$RESULT+36000


while [ $RESULT -le $END ] ; do
	$SHELL_CMD ../stress_test_scn01.1/run.sh
	get_time
done
echo SUCCESS
exit 0

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

# check if the sysfs knobs are in place to turn invariance off
if [ -e "/sys/kernel/hmp/scale_invariant_load" ] ; then
  echo "Prototype HMP-based frequency invariance is present and controllable"
else
  echo "Prototype HMP-based frequency invariance NOT present and controllable"
fi
if [ -e "/sys/devices/system/cpu/cpu0/topology/enable_scaled_cpupower" ] ; then
  echo "Prototype CPU-Power based scale and frequency invariance is present and controllable"
else
  echo "Prototype CPU-Power based scale and frequency invariance NOT present and controllable"
fi


echo "This test always passes, for info only"
echo "Test PASSED"
exit 0



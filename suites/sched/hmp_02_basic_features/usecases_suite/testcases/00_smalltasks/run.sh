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

# Import functions library
source $TOOLS/basics.sh
source $TOOLS/functestlib.sh

RTAPP=$TOOLS/rt-app

cpufreq_governor() {
  CPU=$1
  GOV=$2

  # Save current governors setup
  getcpufreqgovernors
  GOVERNORS=$__RET
  echo "Original governors setting:"
  echo $GOVERNORS

  echo "Calibrate big CPU$CPU..."
  setgovernor_manual $CPU $GOV

  echo "Calibration target CPU setting:"
  grep "" /sys/devices/system/cpu/cpu$CPU/cpufreq/scaling_governor
  grep "" /sys/devices/system/cpu/cpu$CPU/cpufreq/scaling_cur_freq

}

cpufreq_restore() {
  # Recover previous governors setup
  setcpufreqgovernors $GOVERNORS
  getcpufreqgovernors
  GOVERNORS=$__RET
  echo "Recovered governors setting:"
  echo $GOVERNORS
}

rtapp_calibrate() {
  CPU=$1

  # Calibrate RT-App on the big cpu
  sed "s/__CPU__/CPU$CPU/" calibration.json.in > calibration.json
  CALIB=`$RTAPP calibration.json 2>&1 | \
    awk '/pLoad/{gsub(/ns/, "", $5); print $5; exit;}'`

  echo "RT-App calibration for CPU$CPU: $CALIB"
  return $CALIB
}

# Calibrate RT-App on the first LITTLE CPU
getlittlecpulist
for CPU in $__RET; do
  break
done

echo 'Switch to [powersave] governor'
cpufreq_governor $CPU "performance"

echo 'Calibrate RT-App on LITTLE CPUs'
rtapp_calibrate $CPU

# echo "Generate RT-App test configuration"
sed "s/__CALIB__/$CALIB/" mp3.json.in > mp3.json

echo "Start MP3-like workload..."
ftrace_start
$RTAPP mp3.json 2>&1
ftrace_stop

TRACE_TASKS="AudioTick AudioOut_2 AudioTrack gle.mp3.decoder OMXCallbackDisp"
getlittlecpumask
CPUS_MASK="0x$__RET"
USAGE_MIN="98.0"
USAGE_MAX="100.0"
ftrace_check_tasks

# Report tasks residencies on output (i.e. logfile)
cat tasks_residencies.txt

if [ $RESULT = 0 ] ; then
	echo "SUCCESS"
else
	echo "FAILED"
fi
exit $RESULT


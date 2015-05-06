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

reset_tracing()
{
  echo "Resetting Event Tracing"
  echo 0 > ${TRACEDIR}/trace
  echo "Disabling and clearing filters on all events"
  for I in `find ${TRACEDIR}/events/*/enable`; do
    if [ "${I}" != "/sys/kernel/debug/tracing/events/ftrace/enable" ] ; then
        echo 0 > ${I}
    fi
  done
  for I in `find ${TRACEDIR}/events/*/filter`; do
    echo 0 > ${I}
  done
  echo "Resetting trace cpumask"
  echo ff > ${TRACEDIR}/tracing_cpumask
  echo "Setting NOP tracer"
  echo nop > ${TRACEDIR}/current_tracer
}

clear_trace_buffer()
{
  echo 0 > ${TRACEDIR}/trace
}

start_tracing()
{
  local TASK_TRACE_PID=${1}
  local FREQ_CPU=${2}
  # trace events we want
  echo "Enabling sched_task_runnable_ratio event trace for pid==${TASK_TRACE_PID}"
  echo 1 > ${TRACEDIR}/events/sched/sched_task_runnable_ratio/enable
  echo "pid==${TASK_TRACE_PID}" > ${TRACEDIR}/events/sched/sched_task_runnable_ratio/filter
  echo "Enabling cpu_frequency event trace for cpu_id==${FREQ_CPU}"
  echo 1 > ${TRACEDIR}/events/power/cpu_frequency/enable
  echo "cpu_id==${FREQ_CPU}" > ${TRACEDIR}/events/power/cpu_frequency/filter
  # config buffer size
  echo 20000 > ${TRACEDIR}/buffer_size_kb
  # start tracing
  echo "Starting trace"
  echo 1 > ${TRACEDIR}/tracing_on
}

stop_tracing()
{
  echo "Stopping trace"
  echo 0 > ${TRACEDIR}/tracing_on
}

run_test()
{
  if [ -f /sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/cpufreq/scaling_available_frequencies ]; then
    FREQS=`cat /sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/cpufreq/scaling_available_frequencies`
  else
    MINF=`cat /sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/cpufreq/scaling_min_freq`
    MAXF=`cat /sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/cpufreq/scaling_max_freq`
    STPF=$((MINF/10))
    FREQS=`seq $MINF $STPF $MAXF`
  fi
  echo "Testing frequencies: "$FREQS

  clear_trace_buffer
  start_tracing "${TIMERLOOPPID}" "${TARGET_LITTLE_CPU}"
  for I in $FREQS; do
    echo "Freq: $I"
    echo $I > /sys/devices/system/cpu/cpu${TARGET_LITTLE_CPU}/cpufreq/scaling_setspeed
    sleep $STEP_DELAY
  done
  stop_tracing
}

fnmatch()
{
case "$2" in
  $1) return 0 ;;
  *) return 1 ;;
esac ;
}

extract_results()
{
FIRST_BIG_CPU=$1
FIRST_LITTLE_CPU=$2
TARGET_LITTLE_CPU=$3
}

find_big_little()
{
#  FIRST_BIG_CPU= lowest numbered big cpu
#  FIRST_LITTLE_CPU= lowest numbered little cpu
#  TARGET_LITTLE_CPU= highest numbered little cpu, where test runs
local OUTPUT=$( awk '
BEGIN{
first_big=99999
first_little=99999
last_little=-1
processor=-1
}

$1 == "processor" {
processor=$3
}

$2 == "part" {
  if ($4=="0xc0f" || $4=="0xd07") {
    if (first_big > processor)
      first_big = processor
  } else {
    if (first_little > processor)
      first_little = processor
    if (last_little < processor)
      last_little = processor
  }
}

END{
  print first_big,first_little,last_little
}
' /proc/cpuinfo )
extract_results ${OUTPUT}
}

check_userspace_exists()
{
RET="0"
for I in `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors` ; do
  if [ "${I}" == "userspace" ] ; then
    RET="1"
  fi
done
}

userspace_governors()
{
local BIGCPU="cpu${1}"
local LITTLECPU="cpu${2}"
OLD_BIG_GOVERNOR=`cat /sys/devices/system/cpu/${BIGCPU}/cpufreq/scaling_governor`
OLD_LITTLE_GOVERNOR=`cat /sys/devices/system/cpu/${LITTLECPU}/cpufreq/scaling_governor`
echo "Setting governors to userspace"
echo userspace > /sys/devices/system/cpu/${BIGCPU}/cpufreq/scaling_governor
echo userspace > /sys/devices/system/cpu/${LITTLECPU}/cpufreq/scaling_governor

GOVERNORS=`cat /sys/devices/system/cpu/*/cpufreq/scaling_governor`
echo "Current governors: " $GOVERNORS

echo "${BIG_FREQ}" > /sys/devices/system/cpu/${BIGCPU}/cpufreq/scaling_setspeed
echo "${LITTLE_FREQ}" > /sys/devices/system/cpu/${LITTLECPU}/cpufreq/scaling_setspeed

FREQS=`cat /sys/devices/system/cpu/*/cpufreq/scaling_cur_freq`
echo "Current frequencies: " $FREQS
}

# take the list of online CPUs and turn it into a comma separated list of CPUIDs
cpulist_expand()
{
RET=""
OIFS=${IFS}
IFS=","
for TERM in `cat /sys/devices/system/cpu/online` ; do
  if fnmatch '*-*' "${TERM}" ; then
    SIZE=`echo "${TERM}" | awk 'BEGIN { FS="-" } { print $1,$2 }'`
    IFS=${OIFS}
    for CPUID in `seq ${SIZE}` ; do
      if [ ${#RET} == 0 ] ; then
        RET="${CPUID}"
      else
        RET="${RET},${CPUID}"
      fi
    done
  else
    if [ ${#RET} == 0 ] ; then
      RET="${TERM}"
    else
      RET="${RET},${TERM}"
    fi
  fi
  IFS=","
done
IFS=${OIFS}
}

build_cpu_list_without_one_cpu()
{
local MISSINGCPU=${1}
cpulist_expand
LISTIN=${RET}
RET=""
OIFS=${IFS}
IFS=","
for CPUID in ${LISTIN}; do
  if [ "${CPUID}" = "${MISSINGCPU}" ] ; then
    local SKIP=1
  else
    if [ ${#RET} == 0 ] ; then
      RET="${CPUID}"
    else
      RET="${RET},${CPUID}"
    fi
  fi
done
IFS=${OIFS}
}

#supports cpumasks for CPUID 0-31
cpumask_for()
{
local CPUID=${1}
RET="CPUID_OUT_OF_RANGE"
case ${CPUID} in
  "0" )
          RET="0x1";;
  "1" )
          RET="0x2";;
  "2" )
          RET="0x4";;
  "3" )
          RET="0x8";;
  "4" )
         RET="0x10";;
  "5" )
         RET="0x20";;
  "6" )
         RET="0x40";;
  "7" )
         RET="0x80";;
  "8" )
        RET="0x100";;
  "9" )
        RET="0x200";;
  "10" )
        RET="0x400";;
  "11" )
        RET="0x800";;
  "12" )
       RET="0x1000";;
  "13" )
       RET="0x2000";;
  "14" )
       RET="0x4000";;
  "15" )
       RET="0x8000";;
  "16" )
      RET="0x10000";;
  "17" )
      RET="0x20000";;
  "18" )
      RET="0x40000";;
  "19" )
      RET="0x80000";;
  "20" )
     RET="0x100000";;
  "21" )
     RET="0x200000";;
  "22" )
     RET="0x400000";;
  "23" )
     RET="0x800000";;
  "24" )
    RET="0x1000000";;
  "25" )
    RET="0x2000000";;
  "26" )
    RET="0x4000000";;
  "27" )
    RET="0x8000000";;
  "28" )
   RET="0x10000000";;
  "29" )
   RET="0x20000000";;
  "30" )
   RET="0x40000000";;
  "31" )
   RET="0x80000000";;
esac
}

internal_clear_little_cpu()
{
echo "moving tasks off cpu${1}"
build_cpu_list_without_one_cpu ${1}
local LIST=${RET}
echo 0 > /dev/cpuctl/all/cpuset.mems
echo "${LIST}" > /dev/cpuctl/all/cpuset.cpus
for I in `cat /dev/cpuctl/tasks`; do
  echo $I > /dev/cpuctl/all/tasks
done
AFFINITY="cpusets"
}

clear_little_cpu()
{
if [ -d "/dev/cpuctl/all" ] ; then
  echo "cpusets already mounted at /dev/cpuctl"
  internal_clear_little_cpu ${1}
else
  mount -t cgroup -o cpuset none /dev/cpuctl 2>/dev/null
  if [ $? != 0 ] ; then
    echo "cpusets mounting failed. little CPU will not be cleared of tasks"
    AFFINITY="taskset"
  else
    mkdir /dev/cpuctl/all
    internal_clear_little_cpu ${1}
  fi
fi
}

move_load()
{
echo "moving load task (pid $TIMERLOOPPID) onto cpu${1}"
if [ "${AFFINITY}" == "cpusets" ] ; then
  mkdir /dev/cpuctl/cpu${1}
  echo 0 > /dev/cpuctl/cpu${1}/cpuset.mems
  echo ${1} > /dev/cpuctl/cpu${1}/cpuset.cpus
  echo ${TIMERLOOPPID} > /dev/cpuctl/cpu${1}/tasks
else
  cpumask_for ${1}
  $TOOLS/taskset -p ${RET} ${TIMERLOOPPID}
fi
}

build_csv()
{
cat ${1} | awk -v CPU=${2} '
BEGIN{
freq=0
ratio=-1
}

$5 == "sched_task_runnable_ratio:" {
timestamp=substr($4,0,length($4)-1)
temp_ratio=substr($8,7)
if (ratio != temp_ratio) {
  if (freq > 0) {
    print timestamp,freq,temp_ratio
    }
  }
  ratio=temp_ratio
}
$5 == "cpu_frequency:" {
timestamp=substr($4,0,length($4)-1)
temp_cpu=substr($7,8)
if(temp_cpu==CPU) {
  temp_freq=substr($6,7)
  if (freq != temp_freq) {
    if (ratio != -1) {
      print timestamp,temp_freq,ratio
      }
    }
    freq=temp_freq
  }
}
'
}


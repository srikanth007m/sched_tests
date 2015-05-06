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
source $TOOLS/platform.sh

# define variables for SD_ flags, taken from scheduler source code.
# Use with isbitsetinbitfield. True value is 2^n
SD_LOAD_BALANCE=0
SD_LOAD_BALANCE_VAL=1
SD_BALANCE_NEWIDLE=1
SD_BALANCE_NEWIDLE_VAL=2
SD_BALANCE_EXEC=2
SD_BALANCE_EXEC_VAL=4
SD_BALANCE_FORK=3
SD_BALANCE_FORK_VAL=8
SD_BALANCE_WAKE=4
SD_BALANCE_WAKE_VAL=16
SD_WAKE_AFFINE=5
SD_WAKE_AFFINE_VAL=32
SD_PREFER_LOCAL=6
SD_PREFER_LOCAL_VAL=64
SD_SHARE_CPUPOWER=7
SD_SHARE_CPUPOWER_VAL=128
SD_SHARE_PKG_RESOURCES=9
SD_SHARE_PKG_RESOURCES_VAL=512
SD_SERIALIZE=10
SD_SERIALIZE_VAL=1024
SD_ASYM_PACKING=11
SD_ASYM_PACKING_VAL=2048
SD_PREFER_SIBLING=12
SD_PREFER_SIBLING_VAL=4096
SD_OVERLAP=13
SD_OVERLAP_VAL=8192
#define SD_LOAD_BALANCE		0x0001	/* Do load balancing on this domain. */
#define SD_BALANCE_NEWIDLE	0x0002	/* Balance when about to become idle */
#define SD_BALANCE_EXEC		0x0004	/* Balance on exec */
#define SD_BALANCE_FORK		0x0008	/* Balance on fork, clone */
#define SD_BALANCE_WAKE		0x0010  /* Balance on wakeup */
#define SD_WAKE_AFFINE		0x0020	/* Wake task to waking CPU */
#define SD_PREFER_LOCAL		0x0040  /* Prefer to keep tasks local to this domain */
#define SD_SHARE_CPUPOWER	0x0080	/* Domain members share cpu power */
#define SD_SHARE_PKG_RESOURCES	0x0200	/* Domain members share cpu pkg resources */
#define SD_SERIALIZE		0x0400	/* Only a single load balancing instance */
#define SD_ASYM_PACKING		0x0800  /* Place busy groups earlier in the domain */
#define SD_PREFER_SIBLING	0x1000	/* Prefer to place tasks in a sibling domain */
#define SD_OVERLAP		0x2000	/* sched_domains of this level overlap */

#define globals
__TRUE=1
__FALSE=0
__BIGCPUS=""
__LITTLECPUS=""
__CORESFILE="$BASEDIR/cores"
# global return value
__RET=""

#supported methods to get at the domain flags
__DOMAINFLAGS_SCHEDDOMAIN="SCHEDDOMAIN"
# "SCHEDDOMAIN" - uses debug output provided by the kernel when CONFIG_SCHED_DEBUG and CONFIG_SYSCTL are turned on.
#                 The files are available at /proc/sys/kernel/sched_domain/cpu<n>/domain<n>
__DOMAINFLAGS_NONE="NONE"
# "NONE"      - After checking, we have determined that none of the above methods are possible.
__DOMAINFLAGS_METHOD="$__DOMAINFLAGS_NONE"
__CPU_DOMAIN="1"
__MC_DOMAIN="0"

exported_variables_doc()
{
  echo '  $__TRUE=1   returned to indicate true result'
  echo '  $__FALSE=0  returned to indicate false result'
  echo '  $__RET      general purpose variable for returning results'
}

#default value methods
default_big_cpulist()
{
  # This default matches TC2
  # used when all other avenues fail
  __RET=`seq $CONFIG_BIGLITTLE_BIG_CPUS`
}
default_little_cpulist()
{
  # This default matches TC2
  # used when all other avenues fail
  __RET=`seq $CONFIG_BIGLITTLE_LITTLE_CPUS`
}

program_exists()
{
# check if a given program in $1 exists or not
  if ( command -v $1 >/dev/null 2>&1 ); then
    return $__FALSE
  else
    return $__TRUE
  fi
}
program_exists_doc()
{
  echo '  check if a program name, given in $1, exists.'
  echo '  returns $__TRUE if so, or $__FALSE'
}

get_threads()
{
  local __VARNAME=$1
  local PID=$2
  local TMP_TIDS=""
  if [ -d "/proc/$PID" ] ; then
    TMP_TIDS=$( ls "/proc/$PID/task/" )
  fi
  local SKIP=0
  local TIDS=""
  # TMP_TIDS contains the list of thread IDs belonging to the PID
  for i in ${TMP_TIDS}; do
    # ignore the first one
    if [ $SKIP -eq 0 ]; then
      SKIP=1
    else
      # append the others
      TIDS="$TIDS$i "
    fi
  done
  eval $__VARNAME='"$TIDS"'
}
get_threads_doc()
{
  echo '  populate a variable (of name passed in $1) with a list of the thread IDs'
  echo '  associated with a PID given in $2, ignoring the first one'
}

get_param_39()
{
  __RET=${39}
}

get_thread_cpu()
{
  local IN=$( cat /proc/$1/stat )
  # in the stat output, field at array pos 38 contains the CPU that a thread is currently running on
  get_param_39 $IN
}
get_thread_cpu_doc()
{
  echo '  set $__RET to the CPU a given thread (PID/TID in $1) is running on, according to stat'
}

getcpuarray()
{
# open /sys/devices/system/cpu/cpuX and count how many there are
# return a string like "0 1 2 3 4"
  local CPUSTRING=""
  for STR in $( ls -d /sys/devices/system/cpu/cpu* ); do
    # get substring starting after /sys/devices/system/cpu/cpu and filter out
    # freq and idle.
    CPUID=${STR:27}
    if [[ "$CPUID" = 'freq' ||  "$CPUID" = 'idle' ]]; then
      continue
    fi
    # other files belong to CPUs
    CPUSTRING="$CPUSTRING$CPUID "
  done
  __RET=$CPUSTRING
}
getcpuarray_doc()
{
  echo '  set $__RET to a list of CPU numbers present in the system'
  echo '  "" indicates no CPUs found'
  echo '  If you have CPU0 and CPU1, output will be "0 1"'
}

# dectohex echos a number passed as $1 in lowercase hex
dectohex()
{
  __RET=$(printf '%x' "$1")
}
dectohex_doc()
{
  echo '  accepts a decimal number passed in variable $1'
  echo '  sets $__RET to the same value expressed as lower-case hex'
}

hextodec()
{
  local HEX="0x$1"
  __RET=$(printf '%d' "$HEX")
}
hextodec_doc()
{
  echo '  accepts a lower-case hexadecimal number passed in variable $1'
  echo '  sets $__RET to the value as decimal'
}


getlastparam()
{
  local ARGCOUNT=$#
  # ARGCOUNT is number of arguments, deref the number to get correct highest numbered parameter.
  eval __RET=\$$ARGCOUNT
}
getlastparam_doc()
{
  echo '  set $__RET to the last parameter passed into this function'
}

cpustringtobitfield()
{
  # capture whole set of arguments
  local STRING="$*"
  local CPU=""
  local ACCUMULATOR="0"
  for CPU in $STRING; do
    # CPU is a logical CPU number, convert to bitfield
    ACCUMULATOR=$( echo "$ACCUMULATOR + 2^$CPU" | $TOOLS/bc )
  done
  # convert to hex
  dectohex "$ACCUMULATOR"
}
cpustringtobitfield_doc()
{
  echo "  converts a list of CPU numbers to a hex bitfield (lower case)"
  echo '  call like cpustringtobitfield $MYCPUS'
  echo "   the CPU list is treated as separate positional parameters,"
  echo "   concatenated in the function."
  echo '   output returned in $__RET'
}

bitfieldtocpustring()
{
  local TESTBIT=1
  local BITNUMBER=0
  local BITFIELD=$1
  local OUTPUT=""
  # can only look for 31-bits on android
  while [ true ] ; do
    if [ $TESTBIT -le 536870912 ] ; then
      local TEST=$(($TESTBIT & $BITFIELD))
      # if ANDing these bits is equal to $TESTBIT, then we have a match
      if [ $TEST -eq $TESTBIT ] ; then
        OUTPUT="$OUTPUT$BITNUMBER "
        BITFIELD=$(( $BITFIELD - $TESTBIT ))
      fi
      TESTBIT=$(( $TESTBIT * 2 ))
      BITNUMBER=$(( $BITNUMBER + 1 ))
    else
      break
    fi
  done
  __RET=$OUTPUT
}
bitfieldtocpustring_doc()
{
  echo '  converts a lower case hex bitfield ($1) to a list of CPU numbers in $__RET'
}

two_to_the_power_y_long()
{
#  local DEC=( $1 )
  local DEC=$1
  __RET="1"
  while [ "$DEC" -gt 0 ] ; do
    __RET=$(($__RET * 2))
    DEC=$(($DEC - 1))
  done
}

two_to_the_power_y()
{
  case $1 in
  0)
    __RET="1"
    ;;
  1)
    __RET="2"
    ;;
  2)
    __RET="4"
    ;;
  3)
    __RET="8"
    ;;
  4)
    __RET="16"
    ;;
  5)
    __RET="32"
    ;;
  6)
    __RET="64"
    ;;
  7)
    __RET="128"
    ;;
  8)
    __RET="256"
    ;;
  9)
    __RET="512"
    ;;
  10)
    __RET="1024"
    ;;
  11)
    __RET="2048"
    ;;
  12)
    __RET="4096"
    ;;
  13)
    __RET="8192"
    ;;
  14)
    __RET="16384"
    ;;
  15)
    __RET="32768"
    ;;
  *)
    # the special cases handle the first 16 bits in bitfields
    # the function below will calculate up to the shell math limit
    # - presumably, 2^31 for all our platforms is fine
    if [ $1 -gt 31 ] ; then
      __RET="0"
    else
      two_to_the_power_y_long $1
    fi
    ;;
  esac
}

isprecalcbitsetinbitfielddec()
{
  local CPUVAL="$1"
  local BITDEC="$2"
  # bitwise AND the two together
  local TEST=$(($CPUVAL & $BITDEC))
  # check that the bits in CPUVAL are also in BITDEC
  if [ $TEST -eq $CPUVAL ] ; then
    return $__TRUE
  fi
  return $__FALSE
}

isbitsetinbitfielddec()
{
  two_to_the_power_y $1
  local BITFIELD="$__RET"
  if [ $BITFIELD -eq "0" ] ; then
    # no bits set in bitfield..
    return "$__FALSE"
  fi
  isprecalcbitsetinbitfielddec $BITFIELD $2
  return "$?"
}

isbitsetinbitfield()
{
  # convert the bit number into a value
  hextodec $2
  isbitsetinbitfielddec "$1" "$__RET"
  return "$?"
}
isbitsetinbitfield_doc()
{
echo '  returns $__TRUE if the bit number in $1 is present in the (lower case hex) bitfield in $2'
echo '  returns $__FALSE otherwise.'
}

sanitise_large_hex_numbers()
{
  local RESULT=$1
  # only handle up to 32 CPUs to avoid handling >32 bit math elsewhere
  # if we have more than 8 hex digits
  if [ ${#1} -gt 8 ] ; then
    # trim the string, keeping only the last 8 chars
    local OFFSET=$((${#1} - 8))
    RESULT=${1:$OFFSET}
    printf -v __RET '%x' "0x$RESULT"
  else
    __RET=$RESULT
  fi
}
sanitise_large_hex_numbers_doc()
{
  echo '  chop off any hex numbers longer than 8 characters so that they'
  echo '  can be expressed in 32-bits.'
  echo '  puts the result as hex into $__RET'
}

getcpusiblings()
{
  local SIBLINGFILE="/sys/devices/system/cpu/cpu$1/topology/core_siblings"
  local SIBLINGS=$( cat $SIBLINGFILE )
  # on ARM, siblings is usually less than 32-bits. Not so on x86-64.
  sanitise_large_hex_numbers $SIBLINGS
}
getcpusiblings_doc()
{
  echo '  return a bitmask (lowercase hex) of the CPU and siblings'
  echo '  of the CPU in $1 according to sysfs CPU topology'
  echo '  returns in $__RET, and is restricted to 32 CPUs'
}

getcpuscheddomain()
{
  local CPUNAME=cpu$1
  local CORRECT_CPU=0
  local DOMAINNAME=domain$2
  local OLDIFS=$IFS
  __RET=""
  IFS=$'\n'
  for I in $( cat /proc/schedstat ); do
    # iterate through the lines of schedstat in order
    # when we see a cpu<n> line where <n>=$1, move into
    # 'correct_cpu' state, acting like a filter to ignore
    # CPU data we are not interested in.
    if [ ${#I} -ge ${#CPUNAME} ] ; then
      if [ ${I:0:3} == "cpu" ] ; then
        if [ ${I:0:${#CPUNAME}} == "$CPUNAME" ]; then
          CORRECT_CPU="1"
        else
          CORRECT_CPU="0"
        fi
      fi
    fi

    if [ "$CORRECT_CPU" -eq 1 ] ; then
      # we are looking at a schedstat line for the correct CPU, which may be the original cpu<n> line or a domain<n> line
      if [ ${#I} -ge ${#DOMAINNAME} ] ; then
	# enough chars to look at the line
        if [ ${I:0:${#DOMAINNAME}} == "$DOMAINNAME" ]; then
          # correct domain<n> line
          __RET="$I"
          break
        fi
      fi
    fi
  done
  IFS=$OLDIFS
}
getcpuscheddomain_doc()
{
  echo '  set $__RET to the whole line of schedstat output for cpu<$1> domain<$2>'
}

getparam2()
{
  __RET="$2"
}

getcpuscheddomainbitfield()
{
  domainflagssupported
  local BITFIELDS=""
  local DOMAIN="0"
  # while we have something to look at..
  getcpuscheddomain "$1" "$__CPU_DOMAIN"
  # __RET contains complete sched domain output for cpu$1 and domain$2
  if [ ${#__RET} -gt 0 ] ; then
    # if there was some output for this domain, we only want the second field
    getparam2 $__RET
    sanitise_large_hex_numbers "$__RET"
    BITFIELDS="$BITFIELDS$__RET "
  fi
  __RET="$BITFIELDS"
}

getcpuscheddomainbitfield_doc()
{
  echo '  set $__RET to the cpumask (bitwise lowercase hex) of the CPUs participating'
  echo '  in the CPU-level of the scheduler domains attached to CPU $1'
}

getpackagescheddomainbitfield()
{
  domainflagssupported
  local BITFIELDS=""
  # while we have something to look at..
  getcpuscheddomain "$1" "$__MC_DOMAIN"
  if [ ${#__RET} -gt 0 ] ; then
    # if there was some output for this domain, we only want the second field
    getparam2 $__RET
    sanitise_large_hex_numbers "$__RET"
    BITFIELDS="$BITFIELDS$__RET "
  fi
  __RET="$BITFIELDS"
}
getpackagescheddomainbitfield_doc()
{
  echo '  set __$RET to the cpumask (bitwise lowercase hex) of the CPUs participating in'
  echo '  in the multi-thread/multi-cluster level of the scheduler domains'
  echo '  attached to CPU $1'
}

getschedstatversion()
{
  local OLDIFS=$IFS
  IFS=$'\n'
  for I in $( cat /proc/schedstat ); do
    if [ ${I:0:7} == "version" ] ; then
      __RET="${I:8}"
    fi
  done
  IFS=$OLDIFS
}
getschedstatversion_doc()
{
  echo '  set $__RET to the version number of the current kernel schedstat'
}

domainflagssupported()
{
  if [ "$__DOMAINFLAGS_METHOD" == "$__DOMAINFLAGS_NONE" ] ; then
    # we need to check proc/sys/kernel/sched_domain directory exists
    # since it is a good possible source
    if [ -d "/proc/sys/kernel/sched_domain" ] ; then
      __DOMAINFLAGS_METHOD="$__DOMAINFLAGS_SCHEDDOMAIN"
      for DOMAINS in $( ls "/proc/sys/kernel/sched_domain/cpu0/" ) ; do
        case $( cat "/proc/sys/kernel/sched_domain/cpu0/$DOMAINS/name" ) in
        MC)
          __MC_DOMAIN=${DOMAINS##*domain}
          ;;
        CPU)
          __CPU_DOMAIN=${DOMAINS##*domain}
          ;;
        esac
      done
    fi
  fi
  # if we still have no domainflags method set, then we can't do it.
  if [ "$__DOMAINFLAGS_METHOD" == "$__DOMAINFLAGS_NONE" ] ; then
    return "$__FALSE"
  else
    return "$__TRUE"
  fi
}
domainflagssupported_doc()
{
  echo '  returns $__TRUE if this kernel exports sched_domain flag entries'
  echo '  returns $__FALSE if not'
  echo '  The kernel must provide /proc/sys/kernel/sched_domain'
}

getdomainflags_scheddomain()
{
  dectohex $( cat "/proc/sys/kernel/sched_domain/cpu$1/domain$2/flags" )
}

getdomainflags()
{
  __RET=""
  # $1 contains the CPU number
  if [ "$__DOMAINFLAGS_METHOD" == "$__DOMAINFLAGS_SCHEDDOMAIN" ] ; then
    getdomainflags_scheddomain "$1" "$2"
  else
    __RET="$__DOMAINFLAGS_NONE"
  fi
}
getdomainflags_doc()
{
  echo '  set $__RET to the domain flags from sched_domain <$2> of cpu<$1>'
}

getpackagescheddomainflags()
{
  __RET=""
  domainflagssupported
  getdomainflags "$1" "$__MC_DOMAIN"
}
getpackagescheddomainflags_doc()
{
  echo '  set $__RET to the flags parameter of the package-level scheduler domain'
  echo '  attached to CPU $1'
}

getcpuscheddomainflags()
{
  __RET=""
  domainflagssupported
  getdomainflags "$1" "$__CPU_DOMAIN"
}
getcpuscheddomainflags_doc()
{
  echo '  set $__RET to the flags parameter of the CPU-level scheduler domain'
  echo '  attached to CPU $1'
}

getprintableschedflags()
{
  local BITFIELD="0x$1"
  local OUTPUT=""
  isprecalcbitsetinbitfielddec "$SD_LOAD_BALANCE_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_LOAD_BALANCE "
  fi
  isprecalcbitsetinbitfielddec "$SD_BALANCE_NEWIDLE_VAL" "$BITFIELD"
  if [  "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_BALANCE_NEWIDLE "
  fi
  isprecalcbitsetinbitfielddec "$SD_BALANCE_EXEC_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_BALANCE_EXEC "
  fi
  isprecalcbitsetinbitfielddec "$SD_BALANCE_FORK_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_BALANCE_FORK "
  fi
  isprecalcbitsetinbitfielddec "$SD_BALANCE_WAKE_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_BALANCE_WAKE "
  fi
  isprecalcbitsetinbitfielddec "$SD_WAKE_AFFINE_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_WAKE_AFFINE "
  fi
  isprecalcbitsetinbitfielddec "$SD_PREFER_LOCAL_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_PREFER_LOCAL "
  fi
  isprecalcbitsetinbitfielddec "$SD_SHARE_CPUPOWER_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_SHARE_CPUPOWER "
  fi
  isprecalcbitsetinbitfielddec "$SD_SHARE_PKG_RESOURCES_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_SHARE_PKG_RESOURCES "
  fi
  isprecalcbitsetinbitfielddec "$SD_SERIALIZE_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_SERIALIZE "
  fi
  isprecalcbitsetinbitfielddec "$SD_ASYM_PACKING_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_ASYM_PACKING "
  fi
  isprecalcbitsetinbitfielddec "$SD_PREFER_SIBLING_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_PREFER_SIBLING "
  fi
  isprecalcbitsetinbitfielddec "$SD_OVERLAP_VAL" "$BITFIELD"
  if [ "$?" -eq "$__TRUE" ] ; then
    OUTPUT="$OUTPUT""SD_OVERLAP "
  fi
  __RET="$OUTPUT"
}

getcpufreqgovernors()
{
  getcpuarray
  local CPUS=$__RET
  __RET=""
  for I in $CPUS ; do
    local GOVERNOR=$( cat "/sys/devices/system/cpu/cpu$I/cpufreq/scaling_governor" )
    __RET="${__RET}cpu$I $GOVERNOR "
  done
}
getcpufreqgovernors_doc()
{
  echo '  capture the current governors for all CPUs in a string formatted like'
  echo '  cpu0 ondemand cpu1 ondemand cpu2 performance. String returned in $__RET'
}

setcpufreqgovernors()
{
  __RET=""
  if [ "$(id -u)" != "0" ] ; then
    echo "Note: normally you must be ROOT to set CPUfreq governors"
  fi
  local CPU=""
  for I in $* ; do
    if [ ${I:0:3} == "cpu" ] ; then
      CPU=${I:3}
    else
      local RESULT=$( echo "$I" > "/sys/devices/system/cpu/cpu$CPU/cpufreq/scaling_governor" )
      __RET="$__RET$RESULT "
    fi
  done
}
setcpufreqgovernors_doc()
{
  echo '  restore the CPUfreq governor configuration using a string captured with'
  echo '  getcpufreqgovernors'
}

setgovernor_manual()
{
  if [ "$(id -u)" != "0" ] ; then
    echo "Note: normally you must be ROOT to set CPUfreq governors"
  fi
  __RET=$( echo "$2" > "/sys/devices/system/cpu/cpu$1/cpufreq/scaling_governor" )
  if [ $? -ne 0 ] ; then
    echo "Failed to set governor of CPU$1 to $2"
  fi
}
setgovernor_manual_doc()
{
  echo '  set the governor for cpu$1 to $2'
  echo '  any output returned in $__RET'
}

loadcachefile()
{
  echo "Using cached big.LITTLE CPU lists stored in $__CORESFILE"
  local GOTBIG=0
  local OLDIFS=$IFS
  IFS=$'\n'
  for FILE in `cat "$__CORESFILE"`; do
    echo "Reading line :${FILE}"
    if [ $GOTBIG -eq 0 ] ; then
      __BIGCPUS=${FILE}
      GOTBIG=$((GOTBIG+1))
    else
      if [ $GOTBIG -eq 1 ] ; then
        __LITTLECPUS=${FILE}
        GOTBIG=$((GOTBIG+1))
      fi
    fi
  done
  IFS=$OLDIFS
}

populatebiglittlecpulists()
{
  # use cached file so we don't need root more than once
  if [ -e "$__CORESFILE" ] ; then
    loadcachefile
  else
    getcpuarray
    local CPUS=$__RET
    local BIGLIST=""
    local LITTLELIST=""
    echo "Probing system to determine big/little CPUs. Using CPU type as metric."
    # start lookup
    for CPU in ${CPUS} ; do
      echo "Probing CPU${CPU}"
      getcputype $CPU
      echo "  CPU type is $__RET"
      if [ "x$__RET" == "x$CONFIG_TARGET_BIG_CPUPART" ] ; then
        if [ ${#BIGLIST} -eq 0 ] ; then
          getcpusiblings $CPU
          if [ ${#__RET} -gt 0 ] ; then
            bitfieldtocpustring "0x$__RET"
            BIGLIST=$__RET
          fi
        fi
      fi
      if [ "x$__RET" == "x$CONFIG_TARGET_LITTLE_CPUPART" ] ; then
        if [ ${#LITTLELIST} -eq 0 ] ; then
          getcpusiblings $CPU
          if [ ${#__RET} -gt 0 ] ; then
            bitfieldtocpustring "0x$__RET"
            LITTLELIST=$__RET
          fi
        fi
      fi
    done
    __BIGCPUS="$BIGLIST"
    __LITTLECPUS="$LITTLELIST"

    # write out a cache file so we can access it again without needing a root script
    echo "writing big.LITTLE CPU arrays into $__CORESFILE for later use"
    local CORESDIR=${__CORESFILE%/*}
    mkdir -p "$CORESDIR"
    echo "$__BIGCPUS" > $__CORESFILE
    echo "$__LITTLECPUS" >> $__CORESFILE
    cat $__CORESFILE
  fi

  if [ "x$__BIGCPUS" == "x" ]; then
    echo
    echo "ERROR: big CPUs identification FAILED"
    echo "Current SchedTest suite supports only: A7+A15 or A53+A57 big.LITTLE systems"
    echo
  fi
  if [ "x$__LITTLECPUS" == "x" ]; then
    echo
    echo "ERROR: LITTLE CPUs identification FAILED"
    echo "Current SchedTest suite supports only: A7+A15 or A53+A57 big.LITTLE systems"
    echo
  fi
}

getbigcpulist()
{
  if [ "${#__BIGCPUS}" -eq 0 ] ; then
    populatebiglittlecpulists
  fi
  __RET="$__BIGCPUS"
}
getbigcpulist_doc()
{
  echo '  examine output from cpuinfo to find CPU with the highest bogomips value'
  echo '  use CPU topology to generate the list of big CPUs, siblings of that CPU'
  echo '  ASSUMES THAT WE ONLY HAVE TWO TYPES OF CLUSTERS, AND ONLY ONE OF EACH!'
}

getbigcpucount()
{
  getbigcpulist
  local BIG_CPU_LIST=$__RET
  __RET=0;
  for i in $BIG_CPU_LIST
  do
    __RET=$((__RET+1));
  done
}
getbigcpucount_doc()
{
  echo 'returns the total number of big cpus'
}

getbigcpumask()
{
  local CPUMASK=""
  getbigcpulist
  local CPU_LIST=$__RET
  for CPU in ${CPU_LIST}
  do
    two_to_the_power_y $CPU
    CPUMASK=$(($CPUMASK+$__RET))
  done
  dectohex $CPUMASK
  echo $CPUMASK

}
getbigcpumask_doc()
{
  echo 'returns the cpu masks of big core'
}

getlittlecpulist()
{
  if [ "${#__LITTLECPUS}" -eq 0 ] ; then
    populatebiglittlecpulists
  fi
  __RET="$__LITTLECPUS"
}
getlittlecpulist_doc()
{
  echo '  examine output from cpuinfo to find CPU with the lowest bogomips value'
  echo '  use CPU topology to generate the list of little CPUs, siblings of that CPU'
  echo '  ASSUMES THAT WE ONLY HAVE TWO TYPES OF CLUSTERS, AND ONLY ONE OF EACH!'
}

getlittlecpucount()
{
  getlittlecpulist
  local LITTLE_CPU_LIST=$__RET
  __RET=0;
  for i in $LITTLE_CPU_LIST
  do
    __RET=$((__RET+1));
  done
 }
getlittlecpucount_doc()
{
  echo 'returns the total number of little cpus'
}

getlittlecpumask()
{
  local CPUMASK=""
  getlittlecpulist
  local CPU_LIST=$__RET
  for CPU in ${CPU_LIST}
  do
    two_to_the_power_y $CPU
    CPUMASK=$(($CPUMASK+$__RET))
  done
  dectohex $CPUMASK
  echo $CPUMASK

}
getlittlecpumask_doc()
{
  echo 'returns the cpu masks of little cores'
}

printrange()
{
  local STR=""
  local CURRENT=$1
  local END=$2
  # possibly we might pass the range the wrong way around?
  # jsut swap if that happens
  if [ "$CURRENT" -gt "$END" ] ; then
    CURRENT=$2
    END=$1
  fi
  while true; do
    STR="$STR$CURRENT "
    CURRENT=$(( $CURRENT + 1 ))
    if [ "$CURRENT" -gt "$END" ] ; then
      break
    fi
  done
  __RET=$STR
}
printrange_doc()
{
  echo '  Convert a single range string (e.g. 1-4) into a sequence list'
  echo '  in $__RET (like 1 2 3 4).'
}

cpurangetolist()
{
  local LIST=""
  local CPURANGE=$1
  local OLDIFS=$IFS
  IFS=","
  for SEGMENT in $CPURANGE ; do
    # now we either have a number or a range
    local LEFT=$( echo $SEGMENT | sed -rn 's!([0-9]+)-([0-9]+)!\1!p')
    local RIGHT=$( echo $SEGMENT | sed -rn 's!([0-9]+)-([0-9]+)!\2!p')
    local COMBINED="$LEFT$RIGHT"
    if [ ${#COMBINED} -ne 0 ] ; then
      printrange $LEFT $RIGHT
      LIST="${LIST}${__RET}"
    else
      LIST="${LIST}${SEGMENT} "
    fi
  done
  IFS=$OLDIFS
  __RET="$LIST"
}
cpurangetolist_doc()
{
  echo '  Convert a cpu range string (like 0,1-3,10) into a list of'
  echo '  CPUIDs returned in $__RET (like 0 1 2 3 10).'
  echo '  No attempt is made at ordering or removing duplicates.'
}

getbootlogfastcpulist()
{
  __RET=""
  SCHEDFASTCPUS=$( dmesg | grep "fast cpus" )
  if [ ${#SCHEDFASTCPUS} -gt 0 ] ; then
    getlastparam $SCHEDFASTCPUS
    cpurangetolist $__RET
  fi
}
getbootlogfastcpulist_doc()
{
  echo '  Scan the boot log for the HMP Scheduler message indicating'
  echo '  which CPUs are considered to be fast.'
  echo '  Populates $__RET with a list of CPUs.'
}

getbootlogslowcpulist()
{
  __RET=""
  SCHEDSLOWCPUS=$( dmesg | grep "slow cpus" )
  if [ ${#SCHEDSLOWCPUS} -gt 0 ] ; then
    getlastparam $SCHEDSLOWCPUS
    cpurangetolist $__RET
  fi
}
getbootlogslowcpulist_doc()
{
  echo '  Scan the boot log for the HMP Scheduler message indicating'
  echo '  which CPUs are considered to be slow.'
  echo '  Populates $__RET with a list of CPUs.'
}

getcputype_oldcpuinfo()
{
  __RET=""
  local TASKSET="taskset"
  if (! program_exists "taskset" ); then
    TASKSET="$TOOLS/taskset"
  fi
  two_to_the_power_y $1
  local CPUMASK=$__RET
  local OLDAFFINITY=$( $TASKSET -p $$ 2>&1 )
  OLDAFFINITY=$(echo $OLDAFFINITY | sed -rn 's!.*:.(.*)$!\1!p')
  $TASKSET -p $CPUMASK $$ > /dev/null 2>&1
  PROCTYPE=$( cat /proc/cpuinfo | grep "CPU part" )
  PROCTYPE=$( echo $PROCTYPE | sed -rn 's!.*(0x[0-9a-zA-Z]*)!\1!p' )
  $TASKSET -p $OLDAFFINITY $$ >/dev/null 2>&1
  __RET=$PROCTYPE
}
getcputype_newcpuinfo()
{
  __RET=""
  local OLDIFS=$IFS
  local CORRECT_CPU="0"
  IFS=$'\n'
  for LINE in $( cat "/proc/cpuinfo" ) ; do
    # iterate through the lines of cpuinfo in order
    # when we see a processor<tab>: <n> line where <n>=$1, move into
    # 'correct_cpu' state, acting like a filter to ignore
    # CPU data we are not interested in.
    if [ ${LINE:0:9} == "processor" ] ; then
      #echo "examining line ${LINE} to see if ${LINE:12} == $1"
      if [ ${LINE:12} == "$1" ]; then
        CORRECT_CPU="1"
      else
        CORRECT_CPU="0"
      fi
    fi

    if [ "$CORRECT_CPU" -eq 1 ] ; then
      # we are looking at a procinfo line for the correct CPU
      if [ ${LINE:0:8} == "CPU part" ]; then
        echo "examining line ${LINE}"
        __RET="${LINE:11}"
        break
      fi
    fi
  done
  IFS=$OLDIFS
}
getcputype()
{
  local COUNT=$(cat /proc/cpuinfo | grep "CPU part" | wc -l)
  if [ ${COUNT} -gt 1 ] ; then
    getcputype_newcpuinfo $1
  else
    getcputype_oldcpuinfo $1
  fi
}
getcputype_doc()
{
  echo '  Populate $__RET with the CPU type identifier for the CPUID'
  echo '  passed in $1. This is taken from /proc/cpuinfo and relies upon'
  echo '  the current behaviour where cpuinfo shows the info for the'
  echo '  calling CPU.'
  echo '  $__RET==0xc0f for A15, 0xc07 for A7.'
}

TESTLOAD="$TOOLS/tasklibrary"
checktasklibrarycalibration()
{
  # share between test suites if possible
  if [ ! -f $CALIBRATION ] ; then
          $TESTLOAD --calibrate
          mv calib.txt $CALIBRATION
  fi
}

# start a light load thread, 1% of a little CPU.
startlightloadthread()
{
  checktasklibrarycalibration
  $TESTLOAD --calibfile=$CALIBRATION --loadseq=0,l.1 >/dev/null 2>&1 &
  __RET=$!
}
startlightloadthread_doc()
{
  echo '  Start a light load thread, return the PID for it in $__RET'
  echo '  Light load is defined as 1% of a little CPU.'
  echo '  Will perform calibration of the load generator if calibration'
  echo '  data is not found.'
}

startheavyloadthread()
{
  checktasklibrarycalibration
  $TESTLOAD --calibfile=$CALIBRATION --loadseq=0,b.90 >/dev/null 2>&1 &
  __RET=$!
}
startheavyloadthread_doc()
{
  echo '  Start a heavy load thread, return the PID for it in $__RET'
  echo '  Heavy load is defined as 90% of a big CPU.'
  echo '  Will perform calibration of the load generator if calibration'
  echo '  data is not found.'
}


## this doc fn comes last
FUNCTIONS="\
program_exists \
get_threads \
get_thread_cpu \
getcpuarray \
dectohex \
hextodec \
getlastparam \
sanitise_large_hex_numbers \
cpustringtobitfield \
isbitsetinbitfield \
getcpusiblings \
getcpuscheddomainbitfield \
getpackagescheddomainbitfield \
getcpuscheddomainflags \
getpackagescheddomainflags \
getschedstatversion \
domainflagssupported \
getdomainflags \
getbigcpulist \
getbigcpucount \
getbigcpumask \
getlittlecpulist \
getlittlecpucount \
getlittlecpumask \
printrange \
getcpufreqgovernors \
setgovernor_manual \
cpurangetolist \
getbootlogfastcpulist \
getbootlogslowcpulist \
getcputype \
startlightloadthread \
startheavyloadthread \
exported_variables \
"

functestlibdoc()
{
  echo "functestlib.sh"
  echo ""
  echo "Functions:"
  for fn in $FUNCTIONS; do
    echo $fn
    eval $fn"_doc"
    echo ""
  done
  echo "Note, these functions will probably not work with >=32 CPUs"
}

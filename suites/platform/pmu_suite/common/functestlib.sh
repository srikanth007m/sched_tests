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

# Define globals
# Define big.LITTLE PMU types
__PMU_A7=ARMv7_Cortex_A7
__PMU_A15=ARMv7_Cortex_A15
__PMU_CCI=CCI

__PART_A15="0xc0f"
__PART_A7="0xc07"

__CALIB="0"			# calibration run disabled by default
__REPEAT="10"			# repetition value for calibration run

__TRUE="1"
__FALSE="0"

__RESULT="1"			# default test result equal test passed

__PERF="$TOOLS/perf"      # use the private asset as perf binary
__TASKSET="$TOOLS/taskset"
##############################################################################
#
# test_event_not_supported ... test that the value of event is not supported
#
# mandatory  parameter:
#  $1 ... event name
#  $2 ... a stringify'ed array containing the different inputs used to drive
#         the workload. This is used to calculate the expected ratio of the
#         events between different runs.
#
##############################################################################
function test_event_not_supported
{
	# Parameter check
	[ $# -ne 2 ] && { echo "Incorrect usage"; exit 1; }

	local __EVENT=$1
	local __SCALES
	local __VALUE=""
	local __RES=$__FALSE
	local __NOTSPTED="<not supported>"
	local __FILE

	__SCALES=($(echo "$2"))

	for __I in $(seq 0 $(( ${#__SCALES[@]} - 1 )) ); do
	    __SCALE=${__SCALES[$__I]}
	    __FILE="$__SCALE.dat"

	    # Test if result file exist
	    if [ ! -f $__FILE ]; then
		echo "Error: Result file $__FILE does not exist!"
		return $__RES
	    fi

	    # Test that $__EVENT exists in $__FILE
	    __EV=$( grep $__EVENT $__FILE | awk 'BEGIN { FS="," } { print $2 }' )
	    if [ "$__EV" == "" ]; then
	    	echo "Error: $__EVENT not found in $__FILE!"
		return $__RES
	    fi

	    # Test that the value of $__EVENT is $__NOTSPTED in $__FILE
	    __VALUE=$( grep $__EVENT $__FILE | awk 'BEGIN { FS="," } { print $1 }' )
	    if [ "$__VALUE" != "$__NOTSPTED" ]; then
	    	echo "Error: Value of $__EVENT is not $__NOTSPTED!"
		return $__RES
	    fi
	done
	# Use for debug
	# echo "ev=$__EVENT val=$__NOTSPTED"

	__RES=$__TRUE

	return $__RES
}

##############################################################################
#
# test_event ... test if value of event is in withing certain variance of
#                change of magnitude. The function expects files named
#                "$magnitude".dat. e.g., 100.dat, 1000.dat, etc.
#
# mandatory  parameter:
#  $1 ... event name
#  $2 ... a stringify'ed array containing the different inputs used to drive
#         the workload. This is used to calculate the expected ratio of the
#         events between different runs.
#
# optional parameter:
#  $3 ... the variance to test when comparing for event values across magnitude
#         changes in the workload. e.g., 0.1 for 10% variance. The default is
#         0.15
#
##############################################################################
function test_event
{
    local __EVENT=$1
    local __SCALES
    local __MARGIN=${3-0.15}
    local __VALUES
    local __I
    local __RATIO
    local __SCALE_RATIO
    local __LB=$( echo "scale=3; 1 - $__MARGIN" | $TOOLS/bc )
    local __UB=$( echo "scale=3; 1 + $__MARGIN" | $TOOLS/bc )

    __SCALES=($(echo "$2"))

    for __I in $(seq 0 $(( ${#__SCALES[@]} - 1 )) ); do
	__SCALE=${__SCALES[$__I]}
	__FILE="$__SCALE.dat"
	if [ ! -f "$__FILE" ]; then
	    echo "Error: Result file $__FILE does not exist!"
	    return $__FALSE
	fi
	__EV=$( grep $__EVENT $__FILE | awk 'BEGIN { FS="," } { print $2 }' )
	__VALUE=$( grep $__EVENT $__FILE | awk 'BEGIN { FS="," } { print $1 }' )
	if [ "$__EV" == "" ]; then
	    echo "Error: $__EVENT not found in $__FILE!"
	    return $__FALSE
	fi
	if [ $( echo "$__VALUE == 0" | $TOOLS/bc ) -ne 0 ]; then
	    echo "ERROR: VALUE=$__VALUE for EVENT=$__EVENT at SCALE=$__SCALE"
	    return $__FALSE
	fi
	__VALUES[$__I]=$__VALUE
    done

    for __I in $(seq $(( ${#__VALUES[@]} - 1 )) ); do
	__SCALE_RATIO=$( echo "scale=3; ${__SCALES[$__I]} / ${__SCALES[$(( $__I - 1 ))]} " | $TOOLS/bc )
	__RATIO=$( echo "scale=3; ${__VALUES[$__I]} / ${__VALUES[$(( $__I - 1 ))]} / $__SCALE_RATIO" | $TOOLS/bc )

	if [ $( echo "$__RATIO < $__LB" | $TOOLS/bc ) -ne 0 ]; then
	    echo "FAIL: RATIO=$__RATIO should be 1+/-$__MARGIN for EVENT=$__EVENT at SCALE=${__SCALES[$__I]}"
	    return $__FALSE
	elif [ $( echo "$__RATIO > $__UB" | $TOOLS/bc ) -ne 0 ]; then
	    echo "FAIL: RATIO=$__RATIO should be 1+/-$__MARGIN for EVENT=$__EVENT at SCALE=${__SCALES[$__I]}"
	    return $__FALSE
	fi
    done
    return $__TRUE
}


##############################################################################
#
# create_cfg_str ... create the configuration string for one or two PMU's
#
# mandatory  parameter:
#  $1 ... configuration names
#  $2 ... 1. PMU: event configuration values as a string
#  $3 ... 1. PMU: pmu name
#  $4 ... 1. PMU: event prefix
#
# optional parameter:
#  $5 ... 2. PMU: event configuration values as a string
#  $6 ... 2. PMU: pmu name
#  $7 ... 2. PMU: event prefix
#
#  $8 ... event modifier (u ... userland, k ... kernel)
#
# return value:
#  configuration string
#
#  Example:
#
#  CFG_VALS_A7="0xFF 0x08"
#  CFG_VALS_A15="0xFF 0x08"
#  CFG_NAMES="cpu_cycles instructions"
#
#  CFG_STR=$( create_cfg_str "$CFG_NAMES" "$CFG_VALS_A7" "$__PMU_A7" "a7" "$CFG_VALS_A15" "$__PMU_A15" "a15" "k" )
#
#  echo "$CFG_STR"
#
#  ARMv7_Cortex_A7/config=0xFF,name=a7_cpu_cycles/k,ARMv7_Cortex_A7/config \
#  =0x08,name=a7_instructions/k,ARMv7_Cortex_A15/config=0xFF,name= \
#  a15_cpu_cycles/k,ARMv7_Cortex_A15/config=0x08,name=a15_instructions/k
#
##############################################################################
function create_cfg_str
{
	# Parameter check
	[[ $# -lt 4 || $# -eq 6 || $# -gt 8 ]] && { echo "Usage: $0(names vals1 pmu1 pfx1 [vals2 pmu2 pfx2] [mod])"; exit 1; }

	local __NAMES=$1
	local __VALS_1=$2
	local __PMU_1=$3
	local __PFX_1=$4
	local __VALS_2=${5-""}
	local __PMU_2=${6-""}
	local __PFX_2=${7-""}
	local __MOD=""
	[ $# -eq 5 ] && __MOD=$5
	[ $# -eq 8 ] && __MOD=$8
	local __NBRPMUS=1
	[ $# -gt 5 ] && __NBRPMUS=2

	local __CFG_STR_1=""
	local __CFG_STR_2=""
	local __CFG_STR=""

	local __CNT=""

	# Transform strings into arrays
	__I=$((0))
	for __VAL in $__NAMES; do
		__NAMES_ARR[$__I]=$__VAL
		__I=$((__I+1))
	done

	local __I=$((0))
	for __VAL in $__VALS_1; do
		__VALS_1_ARR[$__I]=$__VAL
		__I=$((__I+1))
	done

	if [ $__NBRPMUS -eq 2 ]; then
		__I=$((0))
		for __VAL in $__VALS_2; do
			__VALS_2_ARR[$__I]=$__VAL
			__I=$((__I+1))
		done
	fi

	# Create config string for each pmu
	__CNT=$(( ${#__VALS_1_ARR[@]} -1 ))

	for __I in $(seq 0 $__CNT); do
		__CFG_STR_1=$__CFG_STR_1,$__PMU_1'/config='${__VALS_1_ARR[$__I]}',name='$__PFX_1'_'${__NAMES_ARR[$__I]}'/'$__MOD
	done

	if [ $__NBRPMUS -eq 2 ]; then
		__CNT=$(( ${#__VALS_2_ARR[@]} -1 ))

		for __I in $(seq 0 $__CNT); do
			__CFG_STR_2=$__CFG_STR_2,$__PMU_2'/config='${__VALS_2_ARR[$__I]}',name='$__PFX_2'_'${__NAMES_ARR[$__I]}'/'$__MOD
		done
	fi

	__CFG_STR=$__CFG_STR_1
	[ $__NBRPMUS -eq 2 ] && __CFG_STR=$__CFG_STR$__CFG_STR_2
	__CFG_STR=$( echo ${__CFG_STR:1} )

	echo "$__CFG_STR"
}

##############################################################################
#
# set_cpu_idle ... private function
#
##############################################################################
function set_cpu_idle
{
	local __V=$1

	for __I in $( ls -d /sys/devices/system/cpu/cpu* ); do
		__ID=${__I:27}
		[[ "$__ID" = 'freq' ||  "$__ID" = 'idle' ]] && continue
		echo "$__V"  > /sys/devices/system/cpu/cpu$__ID/cpuidle/state1/disable
	done
}

##############################################################################
#
# enable_cpu_idle ... enable state1 of the cpu idle driver
#
##############################################################################
function enable_cpu_idle
{
	echo "Enabling cpu idle state 1 ... "
	set_cpu_idle 0
}

##############################################################################
#
# disable_cpu_idle ... disable state1 of the cpu idle driver
#
##############################################################################
function disable_cpu_idle
{
	echo "Disabling cpu idle state 1 ... "
	set_cpu_idle 1
}

##############################################################################
#
# build_cpu_mask ... create the CPU mask for $1 by parsing /proc/cpuinfo
#
# mandatory parameter:
#  $1 ... Part ID of the CPU for which mask is required
#
# return value:
#  cpumask for CPUs of type $1
#
##############################################################################
function build_cpu_mask {
    	local PART=$1
	local __CPU_MASK=0
	local IFS
	IFS=$'\n'

	for __LINE in $( cat /proc/cpuinfo ); do
		IFS=':'
		__TOKENS=($__LINE)
		if [ "${__LINE#'processor'}" != "$__LINE" ]; then
			__CPU="${__TOKENS[1]##' '}"
		elif [ "${__LINE#'CPU part'}" != "$__LINE" ]; then
			__PART="${__TOKENS[1]##' '}"
			if [ "$__PART" == "$PART" ]; then
				(( __CPU_MASK |= 1 << $__CPU ))
			fi
		fi
	done
	echo $(printf "0x%X" $__CPU_MASK)
}

##############################################################################
#
# stringify ... convert an input array to string
#
# return value:
#  a string containing space-separated input-array members
#
##############################################################################
function stringify
{
    echo "$@"
}

##############################################################################
#
# find_cpu ... from /proc/cpuinfo find the nth CPU with PART
# mandatory  parameter:
#  $1 ... CPU Part ID
#  $2 ... nth CPU that satisfies $1
#
# return value:
#  The logical CPU number satisfying input constraints
#
##############################################################################
function find_cpu
{
    	local PART=$1
	local CPU=$2
	local IFS
	IFS=$'\n'

	for __LINE in $( cat /proc/cpuinfo ); do
		IFS=':'
		__TOKENS=($__LINE)
		if [ "${__LINE#'processor'}" != "$__LINE" ]; then
			__CPU="${__TOKENS[1]##' '}"
		elif [ "${__LINE#'CPU part'}" != "$__LINE" ]; then
			__PART="${__TOKENS[1]##' '}"
			if [ "$PART" == "$__PART" ]; then
			    CPU=$(( $CPU - 1 ))
			fi
			if [ $CPU -eq 0 ]; then
			    echo $__CPU
			    break
			fi
		fi
	done
}

##############################################################################
#
# hotplug_off_cpu ... Hot(un)plug $1
#
##############################################################################
function hotplug_off_cpu
{
    local __CPU=$1
    echo 0 > /sys/devices/system/cpu/cpu$__CPU/online
}

##############################################################################
#
# hotplug_on_cpu ... Hotplug $1
#
##############################################################################
function hotplug_on_cpu
{
    local __CPU=$1
    echo 1 > /sys/devices/system/cpu/cpu$__CPU/online
}


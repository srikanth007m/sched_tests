SYSFS_DIR=/sys/class/thermal
THERMAL_ZONE_MAX_ID=0
CDEV_PREFIX=cooling_device
TZ_PREFIX=thermal_zone
TRIP_PREFIX=trip_point
STATE_DELAY=1
HIGH_TEMP=80000
LOW_TEMP=18000
CDEV_SYSFS_FILES="cur_state max_state power subsystem type uevent"
TZ_SYSFS_FILES="integral_cutoff k_d k_i k_po k_pu mode policy power \
subsystem sustainable_power temp type uevent"

set_trip_point_temp() {

	if [[ "$1" == "" ]]; then
		echo "Usage: set_trip_point_temp <temp> <id> <tz_id=0>"
		return 1
	else
		TEMP=$1
	fi

	if [[ "$2" == "" ]]; then
		echo "Usage: set_trip_point_temp <temp> <id> <tz_id=0>"
		return 1
	else
		TRIP_ID=$2
	fi

	if [[ "$3" != "" ]]; then
		TZ_ID=$3
	else
		TZ_ID=0
	fi

	TRIP_FILE=$SYSFS_DIR/${TZ_PREFIX}${TZ_ID}/${TRIP_PREFIX}_${TRIP_ID}_temp

	if [[ ! -f $TRIP_FILE ]]; then
		echo "File: $TRIP_FILE not valid"
		return 1
	fi

	echo $TEMP > $TRIP_FILE
	verify_temp=`cat $TRIP_FILE`

	if [[ "$verify_temp" != "$TEMP" ]]; then
		echo "Unable to set temperature"
		return 1
	fi

	return 0

}

get_cdev_max_state() {
	if [[ "$1" == "" ]]; then
		echo "Usage: get_cdev_max_state <cdev_id>"
		return 1
	else
		CDEV_ID=$1
	fi

	CDEV_FILE=$SYSFS_DIR/${CDEV_PREFIX}${CDEV_ID}/max_state

	if [[ ! -f $CDEV_FILE ]]; then
		echo "FILE: $CDEV_FILE not found"
		return 1
	fi

	cat $CDEV_FILE

}

get_cdev_cur_state() {
	if [[ "$1" == "" ]]; then
		echo "Usage: get_cdev_cur_state <cdev_id>"
		return 1
	else
		CDEV_ID=$1
	fi

	CDEV_FILE=$SYSFS_DIR/${CDEV_PREFIX}${CDEV_ID}/cur_state

	if [[ ! -f $CDEV_FILE ]]; then
		echo "FILE: $CDEV_FILE not found"
		return 1
	fi

	cat $CDEV_FILE
}

get_cdev_type() {
	if [[ "$1" == "" ]]; then
		echo "Usage: get_cdev_type <cdev_id>"
		return 1
	else
		CDEV_ID=$1
	fi

	CDEV_FILE=$SYSFS_DIR/${CDEV_PREFIX}${CDEV_ID}/type

	if [[ ! -f $CDEV_FILE ]]; then
		echo "FILE: $CDEV_FILE not found"
		return 1
	fi

	cat $CDEV_FILE
}

is_cpufreq() {
	TYPE=$1
	echo $TYPE | grep -q "cpufreq"
	return $?
}

get_cdev_idx() {

	cdev_idx=()
	cdevs=`ls $SYSFS_DIR/${CDEV_PREFIX}*`
	for cdev in $cdevs; do
		cdev_idx+=(`echo $(basename $cdev) | sed 's/'$CDEV_PREFIX'//g'`)
	done
	unset cdevs
	echo ${cdev_idx[@]}
}

trip_exists() {

	suffices="temp type hyst"
	if [[ "$1" == "" ]]; then
		echo "Usage: trip_exists <trip_id> <tz_id=0>"
		return 1
	else
		TRIP_ID=$1
	fi

	if [[ "$2" != "" ]]; then
		TZ_ID=$2
	else
		TZ_ID=0
	fi

	TRIP_FILE_BASE=$SYSFS_DIR/${TZ_PREFIX}${TZ_ID}/${TRIP_PREFIX}_${TRIP_ID}

	for suffix in $suffices; do
		if [[ ! -f ${TRIP_FILE_BASE}_${suffix} ]]; then
			echo "File: ${TRIP_FILE_BASE}_${suffix} does not exist"
			return 1
		fi
	done
	return 0
}


check_cdev() {
	if [[ "$1" == "" ]]; then
		echo "Usage: check_cdev <cdev_id>"
		return 1
	else
		CDEV_ID=$1
	fi

	CDEV_DIR=$SYSFS_DIR/${CDEV_PREFIX}${CDEV_ID}

	if [[ ! -d $CDEV_DIR ]]; then
		echo "Dir: $CDEV_DIR does not exist"
		return 1
	fi

	for file in $CDEV_SYSFS_FILES; do
		if [[ ! -e $CDEV_DIR/$file ]]; then
			echo "File: $CDEV_DIR/$file does not exist"
			return 1
		fi
	done

	return 0
}

check_tz_sysfs() {
	if [[ "$1" == "" ]]; then
		echo "Usage: check_tz_sysfs <tz_id>"
		return 1
	else
		TZ_ID=$1
	fi

	TZ_DIR=$SYSFS_DIR/${TZ_PREFIX}${TZ_ID}

	if [[ ! -d $TZ_DIR ]]; then
		echo "Dir: $TZ_DIR does not exist"
		return 1
	fi

    if [[ ! -f $TZ_DIR/type ]]; then
        echo "File: $TZ_DIR/type does not exist"
        return 1
    fi

    tz_type=`cat $TZ_DIR/type`

	for file in $TZ_SYSFS_FILES; do
        if [[ "$file" == "mode" && "$tz_type" == "gpu" ]]; then
            continue
        fi

		if [[ ! -e $TZ_DIR/$file ]]; then
			echo "File: $TZ_DIR/$file does not exist"
			return 1
		fi
	done

	return 0
}

check_tz_cdev() {

	cdev_prefix="cdev"
	suffices="trip_point weight"

	if [[ "$1" == "" ]]; then
		echo "Usage: check_tz_cdev <cdev_id> <tz_id=0>"
		return 1
	else
		CDEV_ID=$1
	fi


	if [[ "$2" != "" ]]; then
		TZ_ID=$2
	else
		TZ_ID=0
	fi

	cdev_prefix=${cdev_prefix}$CDEV_ID
	TZ_DIR=${SYSFS_DIR}/${TZ_PREFIX}${TZ_ID}

	if [[ ! -L $TZ_DIR/$cdev_prefix ]]; then
		echo "Link: $TZ_DIR/$cdev_prefix does not exist"
		return 1
	fi

	for suffix in $suffices; do
		if [[ ! -f $TZ_DIR/${cdev_prefix}_$suffix ]]; then
			echo "File: $TZ_DIR/${cdev_prefix}_$suffix does not exist"
			return 1
		fi
	done

	return 0
}


get_trip_id() {

	cdev_prefix="cdev"

	if [[ "$1" == "" ]]; then
		echo "Usage: get_trip_id <cdev_id> <tz_id=0>"
		return 1
	else
		CDEV_ID=$1
	fi

	if [[ "$2" != "" ]]; then
		TZ_ID=$2
	else
		TZ_ID=0
	fi

	TZ_DIR=${SYSFS_DIR}/${TZ_PREFIX}${TZ_ID}
	CDEV_TRIP_FILE=${SYSFS_DIR}/${TZ_PREFIX}${TZ_ID}/${cdev_prefix}${CDEV_ID}_trip_point

	if [[ ! -f $CDEV_TRIP_FILE ]]; then
		echo "File: $CDEV_TRIP_FILE does not exist"
		return 1
	fi

	cat $CDEV_TRIP_FILE
	return 0

}


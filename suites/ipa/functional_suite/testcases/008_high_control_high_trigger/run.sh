# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

# Set the trip points to a "very high" temperature
set_trip_point_temp $HIGH_TEMP 0
set_trip_point_temp $HIGH_TEMP 1
cdevs=`get_cdev_idx`

# Add delay for the state to settle
sleep $STATE_DELAY

for cdev in $cdevs; do
	cur_state=`get_cdev_cur_state $cdev`

	echo "Checking cdev_id: $cdev"
	if [[ $cur_state -ne 0 ]]; then
		echo "Failed: cur_state for cdev_id: $cdev is $cur_state"
		RETURN=$((RETURN + 1))
	else
		echo "Passed: cur_state for cdev_id: $cdev is $cur_state"
	fi

done
exit $RETURN

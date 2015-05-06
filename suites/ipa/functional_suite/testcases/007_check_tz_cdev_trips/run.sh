RETURN=0
CDEVS="0 1 2"
EXPECTED_TRIPS=(1 1 1)

# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

count=0
for cdev_id in $CDEVS; do

	trip_id=`get_trip_id $cdev_id`
	expected_trip_id=${EXPECTED_TRIPS[$count]}

	echo "cdev_id: $cdev_id"
	echo "trip_id: $trip_id"
	echo "expected_trip_id: $expected_trip_id"

	if [[ $expected_trip_id -ne $trip_id ]]; then
		RETURN=$((RETURN + 1))
	fi

	count=$((count + 1))
done

exit $RETURN

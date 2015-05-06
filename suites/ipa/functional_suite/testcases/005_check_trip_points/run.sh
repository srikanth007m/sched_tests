RETURN=0
EXPECTED_TRIPS="0 1"

# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

for trip_id in $EXPECTED_TRIPS; do

	echo "Checking trip_id: $trip_id"
	trip_exists $trip_id
	if [[ $? -ne 0 ]]; then
		echo "trip_id: $trip_id does not exist"
		RETURN=$((RETURN + 1))
	else
		echo "trip_id: $trip_id exists"
	fi
done

exit $RETURN


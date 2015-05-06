RETURN=0
EXPECTED_TZ="0 1"

# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

for tz_id in $EXPECTED_TZ; do

	echo "Checking tz_id: $tz_id"
	check_tz_sysfs $tz_id
	if [[ $? -ne 0 ]]; then
		echo "Failed for tz_id: $tz_id"
		RETURN=$((RETURN + 1))
	else
		echo "Passed for tz_id: $tz_id"
	fi
done

exit $RETURN


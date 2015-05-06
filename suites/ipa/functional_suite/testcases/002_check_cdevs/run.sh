RETURN=0
EXPECTED_CDEVS="0 1 2"

# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

for cdev_id in $EXPECTED_CDEVS; do

	echo "Checking cdev_id: $cdev_id"
	check_cdev $cdev_id
	if [[ $? -ne 0 ]]; then
		echo "Failed for cdev_id: $cdev_id"
		RETURN=$((RETURN + 1))
	else
		echo "Passed for cdev_id: $cdev_id"
	fi
done

exit $RETURN


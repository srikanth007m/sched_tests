RETURN=0
BOUND_CDEVS="0 1 2"

# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

for cdev_id in $BOUND_CDEVS; do

	echo "Checking cdev_id: $cdev_id"
	check_tz_cdev $cdev_id
	if [[ $? -ne 0 ]]; then
		echo "cdev_id: $cdev_id is not bound"
		RETURN=$((RETURN + 1))
	else
		echo "cdev_id: $cdev_id is bound"
	fi
done

exit $RETURN


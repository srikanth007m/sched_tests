NUM_DEVFREQ=0
EXPECTED_DEVFREQ=1
DEVFREQ_TYPE="devfreq"

# Go to the test directory
SOURCE="$0"
RETURN=0
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

cdevs=`get_cdev_idx`

for cdev in $cdevs; do
	type=`get_cdev_type $cdev`

	echo "Checking cdev_id: $cdev"
	if [[ "$type" == $DEVFREQ_TYPE ]]; then
		NUM_DEVFREQ=$((NUM_DEVFREQ + 1))
		echo "cdev_id: $cdev_id is devfreq cdev"
	fi
done

if [[ $NUM_DEVFREQ -eq $EXPECTED_DEVFREQ ]]; then
	echo "Passed"
	exit 0
else
	echo "Failed"
	exit 1
fi

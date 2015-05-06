NUM_CPUFREQ=0
EXPECTED_CPUFREQ=2

# Go to the test directory
SOURCE="$0"
cd `dirname $SOURCE`

# Source the common functions for the test cases
. ../../ipa_test_functions.sh

cdevs=`get_cdev_idx`

for cdev in $cdevs; do
	type=`get_cdev_type $cdev`

	echo "Checking cdev_id: $cdev"
	is_cpufreq $type
	if [[ $? -eq 0 ]]; then
		NUM_CPUFREQ=$((NUM_CPUFREQ + 1))
		echo "cdev_id: $cdev_id is cpufreq cdev"
	fi
done

if [[ $NUM_CPUFREQ -eq $EXPECTED_CPUFREQ ]]; then
	echo "Passed"
	exit 0
else
	echo "Failed"
	exit 1
fi

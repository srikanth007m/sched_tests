The testrunner application is the user entry point for the test harness framework.
The following are some usage examples that showcase the functionality of testrunner:

The following execution will permanently store the given path
as the path of the suite 'example'. Note that full paths are
required.

$ testrunner --add /home/userx/test/examplesuite --suite example

The following execution will display all the existing suites,
known to testrunner.

$ testrunner --list

The following execution will list all usecases inside the suite 'example'.
Note that the suite has to have been added first in order to be visible to testrunner.

$ testrunner --list --suite example

the following execution will run 'testcase0' that belongs to suite 'example'
once:

$ testrunner --run --suite example --testcase testcase0

The following execution will run 10 times every testcase in suite 'example'.

$ for i in `testrunner --list --suite example`; do
  testrunner --run --n 10 --suite example --testcase $i
  done

The following execution prints the description of the testcase tc1 which belongs to
suite 'example':

$ testrunner --desc --suite example --testcase tc1


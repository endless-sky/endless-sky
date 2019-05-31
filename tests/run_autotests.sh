#!/bin/bash

NUM_FAILED=0

# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)

# Determine base path for resources
RESOURCES=$(cd "${HERE}"; cd .. ; pwd)

# Determine path to auto-testers
AUTO_TEST_PATH="${RESOURCES}/tests"

# Determine path to endless-sky executable
ES_EXEC_PATH="${RESOURCES}/endless-sky"

if [ ! -d "${AUTO_TEST_PATH}" ]
then
	echo "No auto-testers directory found. Probably path issue. Returning failure."
	exit 1
fi

if [ ! -f "${ES_EXEC_PATH}" ] || [ ! -x "${ES_EXEC_PATH}" ]
then
	echo "Endless sky executable not found. Returning failure."
	exit 1
fi

TESTS=$(cd "${AUTO_TEST_PATH}"; ls *.txt -1 | sed "s/\.txt\$//")

echo "***********************************************"
echo "***         ES Autotest-runner              ***"
echo "***********************************************"
echo "Resources is ${RESOURCES}"
echo "***********************************************"
echo "Tests to execute:"
echo "${TESTS}"
echo "***********************************************"

# Set separator to newline (in case tests have spaces in their name)
IFS_OLD=${IFS}
IFS="
"

# Run all the tests
for TEST in ${TESTS}
do
	TEST_RESULT="PASS"
	echo "Running test ${TEST}"
	"$ES_EXEC_PATH" --resources "${RESOURCES}" --auto-test "${TEST}"
	if [ $? -ne 0 ]
	then
		TEST_RESULT="FAIL"
		NUM_FAILED=$((NUM_FAILED + 1))
	fi
	echo "Test ${TEST}: ${TEST_RESULT}"
done

IFS=${IFS_OLD}

if [ ${NUM_FAILED} -ne 0 ]
then
	echo "${NUM_FAILED} tests failed"
	exit 1
fi
echo "All executed tests passed"
exit 0

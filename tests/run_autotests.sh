#!/bin/bash

NUM_FAILED=0

# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)

RESOURCES="${HERE}/.."

# Determine path to endless-sky executable
ES_EXEC_PATH="${RESOURCES}/endless-sky"

if [ ! -f "${ES_EXEC_PATH}" ] || [ ! -x "${ES_EXEC_PATH}" ]
then
	echo "Endless sky executable not found. Returning failure."
	exit 1
fi

TESTS=$("${ES_EXEC_PATH}" --list-tests)
TESTS_OK=$(echo "${TESTS}" | grep -e "ACTIVE$" | cut -d$'\t' -f1)
TESTS_NOK=$(echo "${TESTS}" | grep -e "KNOWN FAILURE$" -e "MISSING FEATURE$" | cut -d$'\t' -f1)

echo "***********************************************"
echo "***         ES Autotest-runner              ***"
echo "***********************************************"
echo "Tests not to execute (known failure or missing feature):"
echo "${TESTS_NOK}"
echo ""
echo "Tests to execute:"
echo "${TESTS_OK}"
echo "***********************************************"

# Set separator to newline (in case tests have spaces in their name)
IFS_OLD=${IFS}
IFS="
"

# Run all the tests
for TEST in ${TESTS_OK}
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
	echo ""
done

IFS=${IFS_OLD}

if [ ${NUM_FAILED} -ne 0 ]
then
	echo "${NUM_FAILED} tests failed"
	exit 1
fi
echo "All executed tests passed"
exit 0

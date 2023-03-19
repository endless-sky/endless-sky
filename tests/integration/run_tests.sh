#!/bin/bash
set -eo pipefail

HERE=$(cd `dirname $0` && pwd)

# Helper function to print debugging data for failures on graphical environment.
function print_graphics_data () {
  if [[ -z ${PRINT_GLXINFO} ]]; then
    return;
  fi
# echo "# ***********************************************"
# echo "# OpenGL versions & available extensions:"
# echo "# ***********************************************"
# echo "$(glxinfo | grep -E "OpenGL|GL_" | sed "s/^/# /")"
  echo "# ***********************************************"
  echo "# OpenGL version:"
  echo "# ***********************************************"
  echo "$(glxinfo | grep -E "OpenGL" | sed "s/^/# /")"
  echo "# ***********************************************"
  echo "# Relevant graphics environment variables:"
  echo "# ***********************************************"
  echo "$(env | grep -E "DISPLAY|XDG" | sed "s/^/# /")"
  echo "# ***********************************************"
  echo "# Known X services running:"
  echo "# ***********************************************"
  echo "$(ps -A | grep -E "Xvfb|xserver|Xorg" | sed "s/^/# /")"
  echo "# ***********************************************"
}



# Helper function that checks for retryable issues in the error-log given
# as parameter (and returns them at stdout).
function detect_retryable_issues () {
  grep -Fx -f "${HERE}/retryable_issues.txt" "$1"
}



# Helper function that executes a single test-run.
# Parameters:
# $1 Test(name)
# Return values:
# 0 = success
# 1 = failure
# 2 = fatal failure (should terminate all testing)
# 3 = recoverable failure (could retry)
function run_single_testrun () {
  local TEST="$1"

  # Setup environment for the test
  local ES_CONFIG_PATH=$(mktemp --directory)
  if [ ! $? ]
  then
    echo "not ok ${RUNNING_TEST} Couldn't create temporary directory"
    return 2
  fi

  mkdir -p "${ES_CONFIG_PATH}"
  if ! cp -r ${ES_CONFIG_TEMPLATE_PATH}/* "${ES_CONFIG_PATH}"; then
    echo "not ok ${RUNNING_TEST} Couldn't copy default config data"
    return 2
  fi

  local TEST_NAME=$(echo ${TEST} | sed "s/\"//g")
  local RETURN=0
  echo "# Running test \"${TEST_NAME}\":"
  # Use sed to remove:
  #   - ALSA messages that appear due to missing soundcards in the CI environment
  #   - SDL messages that appear due to limitations of the software renderer
  #   - A VSync message that appears due to no support for native vsync in the software renderer
  #   - A warning that no audio was loaded
  if ! "$ES_EXEC_PATH" --resources "${RESOURCES}" --test "${TEST_NAME}" --config "${ES_CONFIG_PATH}" 2>&1 |\
    sed -e "/^ALSA lib.*$/d" -e "/^AL lib.*$/d" -e "/^(SDL message.*$/d" -e "/^Unable to change VSync.*/d" \
      -e "/^Warning: audio could not.*$/d" | sed "s/^/#     /"
  then
    echo "# Test \"${TEST_NAME}\" failed!"
    echo "#   temporary directory: ${ES_CONFIG_PATH}"
    RETURN=1
    if [ -f "${ES_CONFIG_PATH}/errors.txt" ]
    then
      local KNOWN_ISSUES=$(detect_retryable_issues "${ES_CONFIG_PATH}/errors.txt")
      if [ $(echo "${KNOWN_ISSUES}" | wc -w) -gt 0 ]
      then
        echo "# Failed on known issue:"
        echo "${KNOWN_ISSUES}" | sed "s/^/#     /"
        RETURN=3
      else
        echo "#   errors.txt content:"
        cat "${ES_CONFIG_PATH}/errors.txt" | sed "s/^/#     /"
      fi
    fi
    print_graphics_data
  fi

  return ${RETURN}
}



# Runs a test, including all retries.
function run_test () {
  local RUN_NR=0
  local TEST_RESULT=3
  while [ ${TEST_RESULT} -eq 3 ] && [ ${RUN_NR} -lt 5 ]
  do
    RUN_NR=$((RUN_NR + 1))
    if [ ${RUN_NR} -gt 1 ]
    then
      echo "# Retrying test due to recoverable environment failure"
    fi
    run_single_testrun "${TEST}"
    TEST_RESULT=$?
  done
  return ${TEST_RESULT}
}



# Retrieve parameters that give the executable and datafile-paths.
if [ -z "$1" ] || [ -z "$2" ]; then
  echo "You must supply a path to the binary as an argument,"
  echo "and you must supply a path to the ES resources (data-files), e.g."
  echo "~$ ./tests/integration/run_tests.sh ./endless-sky ./"
  exit 1
fi


# Determine paths to endless-sky executable and other relevant data
ES_EXEC_PATH="$1"
RESOURCES="$2"
ES_CONFIG_TEMPLATE_PATH="${RESOURCES}/tests/integration/config"

echo "TAP version 13"
echo "# ***********************************************"
echo "# ***         ES Autotest-runner              ***"
echo "# ***********************************************"
echo "# Using Test Anything Protocol for reporting test-results"
print_graphics_data

if [ ! -f "${ES_EXEC_PATH}" ]
then
  echo "1..1"
  echo "not ok 1 Endless sky executable not found."
  exit 1
fi

if [ ! -x "${ES_EXEC_PATH}" ]
then
  echo "1..1"
  echo "not ok 1 Endless sky executable not executable. (did you use artifact downloading?)"
  exit 1
fi

# Set separator to newline (in case tests have spaces in their name)
IFS=$'\n'

TESTS=$("${ES_EXEC_PATH}" --tests --resources "${RESOURCES}" --config "${ES_CONFIG_TEMPLATE_PATH}")
TESTS_OK=($(echo "${TESTS}")) || true
NUM_TOTAL=${#TESTS_OK[@]}

if [ ${NUM_TOTAL} -eq 0 ]
then
  echo "1..1"
  echo "not ok 1 Could not find any testcases"
  exit 1
fi

echo "1..${NUM_TOTAL}"

# Run all the tests
RUNNING_TEST=1
NUM_FAILED=0
NUM_OK=0
for TEST in ${TESTS_OK[@]}
do
  if run_test "${TEST}"; then
    NUM_OK=$((NUM_OK + 1))
    TEST_RESULT="ok"
  elif [ $? -eq 2 ]; then
    echo "# Bail out! Encountered serious issue that prevents further testing."
    exit 1
  else
    NUM_FAILED=$((NUM_FAILED + 1))
    TEST_RESULT="not ok"
  fi
  echo "${TEST_RESULT} ${RUNNING_TEST} ${TEST}"
  RUNNING_TEST=$(( ${RUNNING_TEST} + 1 ))
done

unset IFS
echo ""
echo "# tests ${NUM_TOTAL}"
echo "# pass ${NUM_OK}"
if [ ${NUM_FAILED} -ne 0 ]
then
  echo "# failed ${NUM_FAILED}"
  exit 1
fi
exit 0

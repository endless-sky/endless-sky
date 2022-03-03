#!/bin/bash

# Script that checks if the Code::Blocks project is complete.
# TODO: If the project is incomplete generate a patch file to make it complete.

set -u pipefail -e
# Determine path of the current script, and go to the ES project root.
HERE=$(cd `dirname $0` && pwd)
cd ${HERE}/..

ESTOP=$(pwd)
CBPROJECT="${ESTOP}/EndlessSkyLib.cbp"
CBTPROJECT="${ESTOP}/EndlessSkyTests.cbp"

RESULT=0

for FILE in $(find source -type f -not -name WinApp.rc -not -name main.cpp | sed s,^source/,, | sort)
do
  # Check if the file is already in the general Code::Blocks project.
  if ! fgrep -q "${FILE}" "${CBPROJECT}"; then
    if [ $RESULT -ne 1 ]; then
      echo -e "\033[1mMissing files in EndlessSkyLib.cbp:\033[0m"
    fi
    echo -e "${FILE}"
    RESULT=1
  fi
done

for FILE in $(find tests/src/ -type f -name "*.h" -o -name "*.cpp" | sed s,^tests/src/,, | sort)
do
  # Check if the file is already in the test Code::Blocks project.
  if ! fgrep -q "${FILE}" "${CBTPROJECT}"; then
    if [ $RESULT -ne 2 ]; then
      echo -e "\033[1mMissing files in EndlessSkyTests.cbp:\033[0m"
    fi
    echo -e "${FILE}"
    RESULT=2
  fi
done

exit ${RESULT}

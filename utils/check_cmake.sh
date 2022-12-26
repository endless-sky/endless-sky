#!/bin/bash

# Script that checks if the CMake project is complete.
# TODO: If the project is incomplete generate a patch file to make it complete.

set -u pipefail -e
# Determine path of the current script, and go to the ES project root.
HERE=$(cd "$(dirname "$0")" && pwd)
cd "${HERE}"/.. || exit

ESTOP=$(pwd)
LIBLIST="${ESTOP}/source/CMakeLists.txt"
TESTLIST="${ESTOP}/tests/CMakeLists.txt"

RESULT=0

for FILE in $(find source -type f -name "*.h" -o -name "*.cpp" -not -name main.cpp | sed s,^source/,, | sort)
do
  # Check if the file is present in the source list.
  if ! grep -Fq "${FILE}" "${LIBLIST}"; then
    if [ $RESULT -ne 1 ]; then
      echo -e "\033[1mMissing source files in source/CMakeLists.txt:\033[0m"
    fi
    echo -e "${FILE}"
    RESULT=1
  fi
done

for FILE in $(find tests/unit -type f -name "*.h" -o -name "*.cpp" | sed s,^tests/unit/,, | sort)
do
  # Check if the file is present in the source list.
  if ! grep -Fq "${FILE}" "${TESTLIST}"; then
    if [ $RESULT -ne 2 ]; then
      echo -e "\033[1mMissing source files in tests/CMakeLists.txt:\033[0m"
    fi
    echo -e "${FILE}"
    RESULT=2
  fi
done

for FILE in $(find tests/integration/config/plugins/integration-tests/data/tests/ -type f -name "*.txt" | sed s,^tests/integration/config/plugins/integration-tests/data/tests/,, | sort)
do
  # Check if the file is present in the source list.
  if ! grep -Fq "${FILE}" "${TESTLIST}"; then
    if [ $RESULT -ne 2 ]; then
      echo -e "\033[1mMissing data files in tests/CMakeLists.txt:\033[0m"
    fi
    echo -e "${FILE}"
    RESULT=2
  fi
done

exit ${RESULT}

#!/bin/bash

# Script that checks if source and test files found on the filesystem
# are listed in their corresponding CMakeLists.txt files.
# TODO: If the project is incomplete generate a patch file to make it complete.

# --- Strict Mode & Options ---
# Exit on error, treat unset variables as errors, ensure pipeline errors are caught.
set -euo pipefail

# --- Configuration ---
# ANSI codes for bold text
BOLD=$(tput bold 2>/dev/null || printf "\033[1m")
RESET=$(tput sgr0 2>/dev/null || printf "\033[0m")

# Error codes for different sections
ERR_SOURCE_MISSING=1
ERR_TESTS_MISSING=2 # Used for both unit tests and integration data

# --- Path Setup ---
# Determine the absolute path of the current script.
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
# Assume the project root is one level up from the script directory.
PROJECT_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)

# Define CMakeLists.txt paths relative to the project root
LIB_CMAKE_LIST="${PROJECT_ROOT}/source/CMakeLists.txt"
TEST_CMAKE_LIST="${PROJECT_ROOT}/tests/CMakeLists.txt"

# --- Pre-flight Checks ---
if [[ ! -d "${PROJECT_ROOT}/source" ]]; then
    printf "Error: Source directory '%s/source' not found.\n" "${PROJECT_ROOT}" >&2
    exit 1
fi
if [[ ! -d "${PROJECT_ROOT}/tests" ]]; then
    printf "Error: Tests directory '%s/tests' not found.\n" "${PROJECT_ROOT}" >&2
    exit 1
fi
if [[ ! -f "${LIB_CMAKE_LIST}" ]]; then
    printf "Error: Library CMakeLists file not found at '%s'\n" "${LIB_CMAKE_LIST}" >&2
    exit 1
fi
if [[ ! -r "${LIB_CMAKE_LIST}" ]]; then
    printf "Error: Library CMakeLists file not readable at '%s'\n" "${LIB_CMAKE_LIST}" >&2
    exit 1
fi
if [[ ! -f "${TEST_CMAKE_LIST}" ]]; then
    printf "Error: Test CMakeLists file not found at '%s'\n" "${TEST_CMAKE_LIST}" >&2
    exit 1
fi
if [[ ! -r "${TEST_CMAKE_LIST}" ]]; then
    printf "Error: Test CMakeLists file not readable at '%s'\n" "${TEST_CMAKE_LIST}" >&2
    exit 1
fi

# --- Function Definition ---

# Checks if files found by 'find' are present in a given CMakeLists.txt file.
# Arguments:
#   $1: Description (e.g., "source files in source/CMakeLists.txt")
#   $2: Directory to search within (relative to PROJECT_ROOT)
#   $3: CMakeLists.txt file path to check against
#   $4: Error code to return if files are missing
#   $5+: 'find' command arguments (predicates like -name '*.cpp')
check_files_in_list() {
    local description="$1"
    local search_dir="$2"
    local list_file="$3"
    local error_code="$4"
    # All remaining arguments are for find
    shift 4
    local find_args=("$@")

    local found_missing=0
    local header_printed=0
    local file_rel_path=""

    # Use process substitution and null delimiters for safe filename handling
    while IFS= read -r -d $'\0' file_abs_path; do
        # Remove the search directory prefix to get the relative path
        # expected in CMakeLists.txt
        file_rel_path="${file_abs_path#${PROJECT_ROOT}/${search_dir}/}"

        # Check if the relative path is listed literally in the CMake file
        if ! grep -Fq -- "${file_rel_path}" "${list_file}"; then
            found_missing=1
            # Print the header only once for this section
            if [[ ${header_printed} -eq 0 ]]; then
                printf "\n%sMissing %s:%s\n" "${BOLD}" "${description}" "${RESET}"
                header_printed=1
            fi
            printf -- "- %s\n" "${file_rel_path}"
        fi
    done < <(find "${PROJECT_ROOT}/${search_dir}" "${find_args[@]}" -print0 | sort -z)
    # ^^^^ Note: find operates from PROJECT_ROOT/search_dir

    if [[ ${found_missing} -ne 0 ]]; then
        return "${error_code}"
    else
        return 0
    fi
}

# --- Main Execution ---
cd "${PROJECT_ROOT}" || exit 1 # Ensure we are in the project root

overall_result=0 # Stores the highest error code encountered

printf "--- Checking project file consistency ---\n"

# 1. Check source files (.h, .cpp excluding main.cpp) in source/CMakeLists.txt
check_files_in_list \
    "source files in source/CMakeLists.txt" \
    "source" \
    "${LIB_CMAKE_LIST}" \
    "${ERR_SOURCE_MISSING}" \
    \( -name "*.h" -o -name "*.cpp" \) -type f -not -name main.cpp \
    || { current_result=$?; [[ $current_result -gt $overall_result ]] && overall_result=$current_result; }

# 2. Check unit test files (.h, .cpp) in tests/CMakeLists.txt
check_files_in_list \
    "unit test source files in tests/CMakeLists.txt" \
    "tests/unit" \
    "${TEST_CMAKE_LIST}" \
    "${ERR_TESTS_MISSING}" \
    \( -name "*.h" -o -name "*.cpp" \) -type f \
    || { current_result=$?; [[ $current_result -gt $overall_result ]] && overall_result=$current_result; }

# 3. Check integration test data files (.txt) in tests/CMakeLists.txt
INTEGRATION_TEST_DATA_PATH="tests/integration/config/plugins/integration-tests/data/tests"
check_files_in_list \
    "integration test data files in tests/CMakeLists.txt" \
    "${INTEGRATION_TEST_DATA_PATH}" \
    "${TEST_CMAKE_LIST}" \
    "${ERR_TESTS_MISSING}" \
    -name "*.txt" -type f \
    || { current_result=$?; [[ $current_result -gt $overall_result ]] && overall_result=$current_result; }


# --- Report Result ---
printf "\n--- Check complete. ---\n"
if [[ ${overall_result} -eq 0 ]]; then
    printf "All checked files are listed in their respective CMakeLists.txt files.\n"
else
    printf "Found missing files. Please update the CMakeLists.txt file(s).\n"
fi

exit ${overall_result}

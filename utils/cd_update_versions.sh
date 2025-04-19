#!/bin/bash

# Updates version strings via set_version.sh and injects Git info into credits.txt.
# Assumes:
# - Running within a Git repository.
# - 'set_version.sh' is in the same directory as this script.
# - 'credits.txt' is at the root of the Git repository.
# - Required tools: git, perl, date, fmt, tr.

# --- Strict Mode & Error Handling ---
# set -e: Exit immediately if a command exits with a non-zero status.
# set -u: Treat unset variables as an error when substituting.
# set -o pipefail: The return value of a pipeline is the status of the last command
#                  to exit with a non-zero status, or zero if no command exited
#                  with a non-zero status.
set -euo pipefail

# --- Configuration & Paths ---
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SET_VERSION_SCRIPT="${SCRIPT_DIR}/set_version.sh"
CREDITS_FILENAME="credits.txt" # Relative to Git root

# --- Pre-flight Checks ---
echo "--- Running Pre-flight Checks ---"
# Check for required commands
command -v git >/dev/null 2>&1 || { echo >&2 "Error: 'git' command not found. Aborting."; exit 1; }
command -v perl >/dev/null 2>&1 || { echo >&2 "Error: 'perl' command not found. Aborting."; exit 1; }
command -v date >/dev/null 2>&1 || { echo >&2 "Error: 'date' command not found. Aborting."; exit 1; }
command -v fmt >/dev/null 2>&1 || { echo >&2 "Error: 'fmt' command not found. Aborting."; exit 1; }
command -v tr >/dev/null 2>&1 || { echo >&2 "Error: 'tr' command not found. Aborting."; exit 1; }

# Check if we are in a Git repository and get the root directory
GIT_ROOT=$(git rev-parse --show-toplevel) || { echo >&2 "Error: Not a Git repository (or any of the parent directories). Aborting."; exit 1; }
CREDITS_FILE="${GIT_ROOT}/${CREDITS_FILENAME}"

# Check if files exist
if [[ ! -f "${SET_VERSION_SCRIPT}" ]]; then
    echo >&2 "Error: Version setting script not found at '${SET_VERSION_SCRIPT}'. Aborting."
    exit 1
fi
if [[ ! -x "${SET_VERSION_SCRIPT}" ]]; then
    echo >&2 "Error: Version setting script '${SET_VERSION_SCRIPT}' is not executable. Aborting."
    exit 1
fi
if [[ ! -f "${CREDITS_FILE}" ]]; then
    echo >&2 "Error: Credits file not found at '${CREDITS_FILE}'. Aborting."
    exit 1
fi
if [[ ! -w "${CREDITS_FILE}" ]]; then
    echo >&2 "Error: Credits file '${CREDITS_FILE}' is not writable. Aborting."
    exit 1
fi
echo "Checks passed."

# --- Get Git Information ---
echo "--- Retrieving Git Information ---"
CURRENT_COMMIT_HASH=$(git rev-parse --verify HEAD) || { echo >&2 "Error: Failed to get current commit hash."; exit 1; }
BUILD_TIMESTAMP=$(date -u '+%Y-%m-%d %H:%M') # UTC timestamp

echo "Current commit: ${CURRENT_COMMIT_HASH}"
echo "Build timestamp: ${BUILD_TIMESTAMP} UTC"

# --- Run External Version Script ---
echo "--- Running external version setter: ${SET_VERSION_SCRIPT} ---"
"${SET_VERSION_SCRIPT}" "${CURRENT_COMMIT_HASH}"
echo "External version script finished."

# --- Prepare Credit String ---
echo "--- Preparing credit string ---"
# Get formatted log message for the latest commit
# (%h = short hash, %an = author name, %s = subject)
GIT_LOG_INFO=$(git log --pretty="format:(%h)%n%nBuilt: ${BUILD_TIMESTAMP} UTC%nLast change by %an:%n %s" -1) || { echo >&2 "Error: Failed to get git log info."; exit 1; }

# Format the string: wrap lines, replace potentially problematic characters
# - fmt -w 33 -c -s: Wrap lines at ~33 chars, indent paragraphs, split only long lines.
# - tr '|$' _: Replace pipe and dollar sign with underscore (safer for embedding).
# - tr '\"' "'": Replace double quotes with single quotes (safer for embedding).
# Note: Adjust formatting/translation if needed for your specific use case.
FORMATTED_CREDIT_STRING=$(echo "${GIT_LOG_INFO}" | fmt -w 33 -c -s | tr '|$' '_' | tr '\"' "'")
echo "Generated credit string snippet:"
echo "${FORMATTED_CREDIT_STRING}" # Show the user what will be inserted

# --- Update Credits File ---
echo "--- Updating ${CREDITS_FILE} ---"
CREDITS_BACKUP="${CREDITS_FILE}.bak"

# Use Perl for in-place substitution.
# Pass the credit string via an environment variable for safety.
# The regex finds a line starting with "version " followed by digits/dots and
# an optional hyphenated suffix (e.g., "version 1.2.3", "version 0.9.1-alpha").
# It then appends the formatted credit string ($ENV{PERL_CREDIT_STRING})
# immediately after the matched version string ($1).
PERL_CREDIT_STRING="${FORMATTED_CREDIT_STRING}" \
perl -p -i -e '
  # s|PATTERN|REPLACEMENT|g; - substitute globally on the line
  # PATTERN: (version [\d.]+(-\w*)?)
  #   (...) - Capture group 1
  #   version  - Literal "version "
  #   [\d.]+   - One or more digits or dots (the version number)
  #   (...)?   - Optional capture group 2
  #   -\w* - Hyphen followed by zero or more word characters (tag)
  # REPLACEMENT: $1$ENV{PERL_CREDIT_STRING}
  #   $1 - The entire matched version string (group 1)
  #   $ENV{PERL_CREDIT_STRING} - The credit string passed via environment
  s|(version [\d.]+(-\w*)?)|$1$ENV{PERL_CREDIT_STRING}|;
' "${CREDITS_FILE}"

# Remove the backup file created by perl -i
if [[ -f "${CREDITS_BACKUP}" ]]; then
    echo "Removing backup file: ${CREDITS_BACKUP}"
    rm -f "${CREDITS_BACKUP}"
else
    echo "Perl did not create a backup file (or it was already removed)."
fi

echo "--- Successfully updated ${CREDITS_FILE} ---"
exit 0

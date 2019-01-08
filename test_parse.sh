#!/bin/bash
if [ -z "$1" ]; then
  exit 1
fi
# OS Check
if [[ $(uname) == 'Darwin' ]]; then
  FILEDIR="$HOME/Library/Application Support/endless-sky"
else
  FILEDIR="$HOME/.local/share/endless-sky/"
fi
ERR_FILE="$FILEDIR/errors.txt"

# Remove any existing error files first.
if [ -f "$ERR_FILE" ]; then
  echo "Removing existing error file"
  rm "$ERR_FILE"
fi

# Parse the game data files (and print to the screen the results).
"$1" -p
EXIT_CODE=$?

# If the game executed, then assert there is no 'errors.txt' file.
if [ $EXIT_CODE -ne 0 ]; then
  echo "Error executing file/command '$1'"
  exit $EXIT_CODE
elif [ -f "$ERR_FILE" ]; then
  echo "Assertion failed: parsing files created file 'errors.txt' in $FILEDIR"
  exit 1
else
  echo "Parse test completed successfully."
fi

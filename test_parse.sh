#!/bin/bash
if [ -z "$1" ]; then
  exit 1
fi
# Mark the input argument as executable
chmod +x "$1"

FILEDIR="$HOME/.local/share/endless-sky/"
ERR_FILE="$FILEDIR/errors.txt"
# Remove any existing error files first.
if [ -f "$ERR_FILE" ]; then
  echo "Removing existing error file"
  rm "$ERR_FILE"
fi
# Parse the game data files (and print to the screen the results).
"$1" -p
# Assert there is no "errors.txt" file
if [ -f "$ERR_FILE" ]; then
  echo "Assertion failed: parsing files created file 'errors.txt' in $FILEDIR"
  exit 1
fi

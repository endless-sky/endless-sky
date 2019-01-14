#!/bin/bash
if [ -z "$1" ]; then
  exit 1
fi
# OS Check
if [[ $(uname) == 'Darwin' ]]; then
  FILEDIR="$HOME/Library/Application Support/endless-sky"
else
  FILEDIR="$HOME/.local/share/endless-sky"
fi
mkdir -p "$FILEDIR"
ERR_FILE="$FILEDIR/errors.txt"

# Remove any existing error files first.
if [ -f "$ERR_FILE" ]; then
  rm "$ERR_FILE"
fi

# Parse the game data files.
"$1" -p 2>"$ERR_FILE"
EXIT_CODE=$?

# If the game executed, then assert there is no 'errors.txt' file.
if [ $EXIT_CODE -ne 0 ]; then
  echo "Error executing file/command '$1'"
  exit $EXIT_CODE
elif [ -f "$ERR_FILE" ] && [ -s "$ERR_FILE" ]; then
  echo "Assertion failed: content written to $ERR_FILE"
  cat "$ERR_FILE"
  exit 1
else
  echo "Parse test completed successfully."
fi

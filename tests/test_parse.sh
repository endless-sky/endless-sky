#!/bin/bash
if [ -z "$1" ]; then
  echo "You must supply a path to the binary as an argument, e.g."
  echo "~$ ./tests/test_parse.sh ./endless-sky"
  exit 1
fi
# OS Check
if [[ $(uname) == 'Darwin' ]]; then
  FILEDIR="$HOME/Library/Application Support/endless-sky"
elif [[ $OSTYPE == 'msys' ]] || [[ $OS == 'Windows_NT' ]] || [[ ! -z ${APPDATA:-} ]]; then
  FILEDIR="$APPDATA/endless-sky"
else
  FILEDIR="$HOME/.local/share/endless-sky"
fi
ERR_FILE="$FILEDIR/errors.txt"
RUNTIME_ERRS="launch-errors.txt"
# Remove any existing error files first.
if [ -f "$ERR_FILE" ]; then
  rm -f "$ERR_FILE"
fi

# Parse the game data files.
"$1" -p 2>"$RUNTIME_ERRS"
EXIT_CODE=$?

# If the game executed, then assert there is no 'errors.txt' file,
# or that it is an empty file.
if [ $EXIT_CODE -ne 0 ]; then
  echo "Error executing file/command '$1':"
  cat "$RUNTIME_ERRS"
  rm -f "$RUNTIME_ERRS"
  exit $EXIT_CODE
elif [ -f "$ERR_FILE" ] && [ -s "$ERR_FILE" ]; then
  cat "$ERR_FILE"
  echo && echo "Assertion failed: content written to $ERR_FILE" && echo
  exit 1
else
  echo "Parse test completed successfully."
fi

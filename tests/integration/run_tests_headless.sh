#!/bin/sh
set -e

# Run headless "acceptance tests" using Xvfb
# These tests involve booting the game, loading particular saved games, and making assertions
# about the state of the game universe. Some tests will involve issuing basic commands.

# TODO: When this script is made cross-platform, replace this OS check with a check for Xvfb and other required resources.
if [ "$OSTYPE" = 'msys' ] || [ "$OS" = 'Windows_NT' ] || [ "$(uname)" = 'Darwin' ]; then
  echo "Headless testing is not supported on this platform"
  exit 126
fi

# Determine path of the current script and change to current working-dir
HERE=$(cd `dirname $0` && pwd)
cd "${HERE}"

# Relative paths for linux headless testing relative to this script.
EXECUTABLE="../../endless-sky"
RESOURCES="../../"

if ! command -v xvfb-run > /dev/null 2>&1; then
  echo "You must install Xvfb to run headless tests"
  exit 127
fi

exec xvfb-run --auto-servernum --server-args="+extension GLX +render -noreset" ./run_tests.sh "${EXECUTABLE}" "${RESOURCES}"

#!/bin/bash
set -e

# Run headless "acceptance tests" using Xvfb
# These tests involve booting the game, loading particular saved games, and making assertions
# about the state of the game universe. Some tests will involve issuing basic commands.

# TODO: When this script is made cross-platform, replace this OS check with a check for Xvfb and other required resources.
if [[ $OSTYPE == 'msys' ]] || [[ $OS == 'Windows_NT' ]] || [[ $(uname) == 'Darwin' ]]; then
  echo "Headless testing is not supported on this platform"
  exit 126
fi

# Determine path of the current script and change to current working-dir
HERE=$(cd `dirname $0` && pwd)
cd "${HERE}"

# Relative paths for linux headless testing relative to this script.
EXECUTABLE="../endless-sky"
RESOURCES="../"

if [ -z "$XDG_RUNTIME_DIR" ];
then
    export XDG_RUNTIME_DIR=/run/user/$(id -u)
fi

if ! touch $XDG_RUNTIME_DIR/.can_i_write 2>/dev/null;
then
    export XDG_RUNTIME_DIR=~/.run/user/$(id -u)
    mkdir -p $XDG_RUNTIME_DIR
    chmod 700 $XDG_RUNTIME_DIR
fi

if ! command -v xvfb-run &> /dev/null; then
  echo "You must install Xvfb to run headless tests"
  exit 127
fi

# Force OpenGL software mode
export LIBGL_ALWAYS_SOFTWARE=1

xvfb-run --auto-servernum ./run_tests.sh "${EXECUTABLE}" "${RESOURCES}"

RETURN_VALUE=$?

kill -s SIGTERM ${XSERVER_PID}

exit ${RETURN_VALUE}

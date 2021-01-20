#!/bin/bash
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


Xvfb :99 -screen 0 1280x1024x24 &
XSERVER_PID=$!
if ! ps -p ${XSERVER_PID} > /dev/null 2>&1; then
	echo "You must install Xvfb to run headless tests"
	exit 127
fi
echo "XServer PID: ${XSERVER_PID}"
export DISPLAY=:99

# Force OpenGL software mode
export LIBGL_ALWAYS_SOFTWARE=1

# Use the query for OpenGL settings to check if the XServer runs
function has_no_display() {
	if glxinfo > /dev/null 2>&1; then
		return 1
	else
		return 0
	fi
}
MAX_RETRY=15
while has_no_display && [[ ${MAX_RETRY} -ge "0" ]]
do
	sleep 1
	echo "Waiting for start of xserver (max-retry=${MAX_RETRY})"
	MAX_RETRY=$(( MAX_RETRY - 1 ))
done
if (( MAX_RETRY < 0 )); then
	echo "Error: Xserver did not start within waiting time"
	exit 126
fi

# Enable for debugging (and add some secret password file to make it more secure):
#
# x11vnc -display :99 &
# X11VNC_PID=$!
# echo "X11VNC PID: ${X11VNC_PID}"

./run_tests.sh "${EXECUTABLE}" "${RESOURCES}"

RETURN_VALUE=$?

kill -s SIGTERM ${XSERVER_PID}

# Enable for debugging:
# kill -s SIGTERM ${X11VNC_PID}

exit ${RETURN_VALUE}

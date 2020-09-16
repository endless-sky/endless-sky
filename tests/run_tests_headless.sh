#!/bin/bash
# TODO: When this script is made cross-platform, replace this OS check with a check for Xvfb and other required resources.
if [[ $OSTYPE == 'msys' ]] || [[ $OS == 'Windows_NT' ]] || [[ $(uname) == 'Darwin' ]]; then
	echo "Headless testing is not supported on this platform"
	exit 1
fi

HERE=$(cd `dirname $0` && pwd)
cd "${HERE}"

# Running headless using Xvfb
Xvfb :99 -screen 0 1280x1024x24 &
XSERVER_PID=$!
echo "XServer PID: ${XSERVER_PID}"
export DISPLAY=:99

# Force openGL software mode
export LIBGL_ALWAYS_SOFTWARE=1

# Use the query for OpenGL settings to check if the XServer runs
MAX_RETRY=15
GLXINFO=$(glxinfo 2>/dev/null)
RETURN_VALUE=$?
while [ "${RETURN_VALUE}" -ne "0" ] && [ "${MAX_RETRY}" -ge "0" ]
do
	sleep 1
	echo "Waiting for start of xserver (max-retry=${MAX_RETRY})"
	MAX_RETRY=$(( MAX_RETRY - 1 ))
	# Use the query for OpenGL settings to check if the XServer runs
	GLXINFO=$(glxinfo 2>/dev/null)
	RETURN_VALUE=$?
done
if [ "${RETURN_VALUE}" -ne "0" ]
then
	echo "Error: Xserver did not start within waiting time"
	exit ${RETURN_VALUE}
fi

echo "OpenGL settings"
echo "${GLXINFO}" | grep OpenGL

# Enable for debugging (and add some secret password file to make it more secure):
#
# x11vnc -display :99 &
# X11VNC_PID=$!
# echo "X11VNC PID: ${X11VNC_PID}"

../endless-sky --test "empty_testcase"
RETURN_VALUE=$?

kill -s SIGTERM ${XSERVER_PID}

# Enable for debugging:
# kill -s SIGTERM ${X11VNC_PID}

exit $RETURN_VALUE

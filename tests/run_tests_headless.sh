#!/bin/bash

HERE=$(cd `dirname $0` && pwd)

cd "${HERE}"

Xvfb :99 -screen 0 1280x1024x24 &
XVFB_PID=$!
echo "XVFB PID: ${XVFB_PID}"
export DISPLAY=:99

# Enable for debugging (and add some secret password file to make it more secure):
#
# x11vnc -display :99 &
# X11VNC_PID=$!
# echo "X11VNC PID: ${X11VNC_PID}"

./run_tests.sh

kill -s SIGTERM ${XVFB_PID}

# Enable for debugging:
# kill -s SIGTERM ${X11VNC_PID}

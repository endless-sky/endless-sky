#!/bin/bash

HERE=$(cd `dirname $0` && pwd)

cd "${HERE}"
# Running headless using Xorg or Xvfb (only choose one)
# Xorg -noreset +extension GLX +extension RANDR +extension RENDER -logfile ./Xserver.log -config ./xorg_dummy.conf :99 &
Xvfb :99 -screen 0 1280x1024x24 &

XSERVER_PID=$!
echo "XServer PID: ${XSERVER_PID}"
export DISPLAY=:99
# Force openGL software mode
export LIBGL_ALWAYS_SOFTWARE=1

echo "OpenGL settings"
glxinfo | grep OpenGL

# Enable for debugging (and add some secret password file to make it more secure):
#
# x11vnc -display :99 &
# X11VNC_PID=$!
# echo "X11VNC PID: ${X11VNC_PID}"

./run_tests.sh
RETURN_VALUE=$?



kill -s SIGTERM ${XSERVER_PID}

# Enable for debugging:
# kill -s SIGTERM ${X11VNC_PID}

exit $RETURN_VALUE

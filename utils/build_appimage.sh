#!/bin/bash

# Builds Endless Sky and packages it as AppImage
# Control the output filename with the OUTPUT environment variable
# You may have to set the ARCH environment variable to e.g. x86_64.

# We need an icon file with a name matching the executable
cp icons/icon_512x512.png endless-sky.png

# Build
scons -Qj $(nproc) install DESTDIR=AppDir

# Inside an AppImage, the executable is a link called "AppRun" at the root of AppDir/.
# Keeping the data files next to the executable is perfectly valid, so we just move them to AppDir/ to avoid errors.
mv AppDir/usr/local/share/games/endless-sky/* AppDir/

# Now build the actual AppImage
curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -o linuxdeploy && chmod +x linuxdeploy
./linuxdeploy --appdir AppDir -e endless-sky -d endless-sky.desktop -i endless-sky.png --output appimage

# Clean up
rm -rf AppDir linuxdeploy endless-sky.png

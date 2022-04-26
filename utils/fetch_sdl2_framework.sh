#!/bin/bash
set -e

# Download and extract the SDL v2 framework for macOS.

SDL2_VERSION="2.0.14"
SDL2_URL="https://www.libsdl.org/release/SDL2-${SDL2_VERSION}.dmg"
SDL2_DMG="${TEMP_ROOT}/sdl2.dmg"

if [ ! -d "${PROJECT_DIR}/build/SDL2.framework" ]; then
    mkdir -p "${PROJECT_DIR}/build"
    if [ ! -f "${SDL2_DMG}" ]; then
        curl -sSL -o "${SDL2_DMG}" "${SDL2_URL}"
    fi
    hdiutil attach -quiet -nobrowse "${SDL2_DMG}"
    cp -R /Volumes/SDL2/SDL2.framework "${PROJECT_DIR}/build"
    hdiutil detach -quiet /Volumes/SDL2
fi

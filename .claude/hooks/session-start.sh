#!/bin/bash
# Installs the libraries needed to build and test Endless Sky in Claude Code cloud sessions.
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
	exit 0
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq || true
apt-get install -y -qq \
	g++ cmake ninja-build ccache pkg-config xvfb \
	libsdl2-dev libpng-dev libjpeg-dev libavif-dev libgl1-mesa-dev libglew-dev \
	libminizip-dev libopenal-dev libmad0-dev libflac++-dev uuid-dev catch2 >/dev/null
pip install -q --break-system-packages regex python-debian >/dev/null 2>&1 || true

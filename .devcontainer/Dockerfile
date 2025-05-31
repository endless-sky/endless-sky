FROM ubuntu:24.04

VOLUME /tmp
VOLUME /var/cache/apt/archives

ENV DEBIAN_FRONTEND=noninteractive

# Basic build and system requirements
RUN \
	apt-get update; \
	apt-get install -y --no-install-recommends cmake pkg-config g++ ninja-build curl git; \
	apt-get install -y --no-install-recommends libgl1-mesa-dev libglu1-mesa-dev; \
	apt-get install -y --no-install-recommends libx11-dev libxft-dev libxext-dev

# Game dependencies
RUN \
	apt-get update; \
	apt-get install -y --no-install-recommends libsdl2-dev libpng-dev libjpeg-dev libglew-dev libminizip-dev libopenal-dev libmad0-dev uuid-dev; \
	apt-get install -y --no-install-recommends pkgconf catch2

# Optional dependencies
RUN \
	apt-get update; \
	apt-get install -y --no-install-recommends python3 python3-regex; \
	apt-get install -y --no-install-recommends rsync gnupg2; \
	apt-get install -y --no-install-recommends xvfb; \
	apt-get install -y --no-install-recommends clang-format gdb

# Build caching
RUN \
	apt-get update; \
	apt-get install -y --no-install-recommends ccache
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_BUILD_PARALLEL_LEVEL=32

CMD /bin/bash

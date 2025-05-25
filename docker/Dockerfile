FROM ubuntu:24.04

VOLUME /tmp
VOLUME /var/cache/apt/archives

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-exo", "pipefail", "-c"]

# Basic build and system requirements
RUN \
	apt-get update; \
	apt-get install -y --no-install-recommends cmake pkg-config g++ ninja; \
	apt-get install -y --no-install-recommends libgl1-mesa-dev libglu1-mesa-dev; \
	apt-get install -y --no-install-recommends libx11-dev libxft-dev libxext-dev

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
	apt-get install ccache; \
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_BUILD_PARALLEL_LEVEL=32

# Create container user (assumes UID 1000)
RUN \
  adduser es-user; \
  usermod -a -G audio es-user

USER es-user
WORKDIR /home/es-user

CMD /bin/bash

#!/bin/bash
set -e

# Build a customized backwards-compatible version of libmad for macOS.

brew install --formula -s "${SRCROOT}/utils/mad-compat.rb"
mkdir -p "${PROJECT_DIR}/build"
cp -f `brew --prefix`/opt/mad-compat/lib/libmad.0.dylib "${PROJECT_DIR}/build"

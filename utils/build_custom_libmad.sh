#!/bin/bash
set -e

# Build a customized backwards-compatible version of libmad for macOS.
if ! type brew &> /dev/null; then
  /opt/homebrew/bin/brew install --formula -s "${SRCROOT}/utils/mad-compat.rb"
  mkdir -p "${PROJECT_DIR}/build"
  cp -f /opt/homebrew/opt/mad-compat/lib/libmad.0.dylib "${PROJECT_DIR}/build"
else
  brew install --formula -s "${SRCROOT}/utils/mad-compat.rb"
  mkdir -p "${PROJECT_DIR}/build"
  cp -f `brew --prefix`/opt/mad-compat/lib/libmad.0.dylib "${PROJECT_DIR}/build"
fi

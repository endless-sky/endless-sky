#!/bin/bash

# Script that looks for shader files in sub

failed=0
# Sets the failed status globally
fail() {
  failed=1
  export failed
}

# Validates a GLSL file.
# Parameters:
# - glsl version
# - filename
check() {
  # Prepend the version number to file, but preserve a copy of the original
  sed -i.old "1s/^/#version $1\n/" "$2"
  glslangValidator --enhanced-msgs "$2" || fail
  rm "$2"
  mv "$2.old" "$2"
}

# Validates a list of GLSL files
# parameters:
# - glsl version
# - filenames...
check_files() {
  for file in "${@:2}"; do
    check "$1" "$file"
  done
}

# Check glob options (case-insensitive, remove unmatched patterns, allow *)
shopt -s nocaseglob
shopt -s nullglob
shopt -s globstar

echo "-------------------"
echo "Checking OpenGL 3.0"
echo "-------------------"
echo

check_files "130" ./**/*.{frag,vert} ./**/*.{frag,vert}.gl

echo "-------------------"
echo "Checking OpenGL ES 3.0"
echo "-------------------"
echo

check_files "300 es" ./**/*.{frag,vert} ./**/*.{frag,vert}.gles

exit $failed

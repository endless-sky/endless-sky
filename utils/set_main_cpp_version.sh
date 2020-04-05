#!/bin/bash

# Sets the version display in main.cpp to the first argument.
# Example: 
# $ ./utils/set_version.sh "my cool version"

perl -p -i -e "s/(Endless Sky) [\d.]+/\$1 $1/ig" source/main.cpp
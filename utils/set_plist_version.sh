#!/bin/bash

# Sets the application version in EndlessSky-Info.plist to the first argument. It should be an integer, e.g. a build number.
# Example: 
# $ ./utils/set_plist_version.sh 1337

perl -0777 -p -i -e "s/(CFBundleVersion.*?)\>\d+/\$1>$1/igs" XCode/EndlessSky-Info.plist
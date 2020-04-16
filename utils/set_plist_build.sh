#!/bin/bash

# Sets the application build number in EndlessSky-Info.plist to the first argument. It should be an integer.
# Example:
# $ ./utils/set_plist_build.sh 1337

perl -0777 -p -i -e "s/(CFBundleVersion.*?)\>\d+/\$1>$1/igs" XCode/EndlessSky-Info.plist
rm -rf XCode/EndlessSky-Info.plist.bak

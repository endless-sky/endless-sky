#!/bin/bash

# Sets the application version to the first argument.

# Example:
# $ ./utils/set_version.sh "my cool version"
# $ ./utils/set_version.sh 1.0.0

versionRegex="([0-9]+\.){0,2}[0-9]+"
[[ $1 =~ ^${versionRegex}$ ]]
isVersion=$?
if [[ ${isVersion} -eq 0 ]]; then
    replacement="ver. $1"
elif [[ $1 =~ ^[0-9A-Fa-f]+$ ]]; then
    replacement="commit $1"
else
    replacement=$1
fi

# Update main.cpp
perl -p -i -e "s/(\"Endless Sky) .+?\"/\$1 ${replacement}\"/ig" source/main.cpp
rm -rf source/main.cpp.bak

# Update manpage.
perl -p -i -e "s/\".*?\" \"ver\. .+?\"/\"$(date '+%d %b %Y')\" \"${replacement}\"/ig" endless-sky.6
rm -rf endless-sky.6.bak

# Update the XCode plist version, if the input is spec-conformant
if [[ ${isVersion} -eq 0 ]]; then
    perl -0777 -p -i -e "s/(CFBundleShortVersionString.*?)\>${versionRegex}/\$1>$1/igs" XCode/EndlessSky-Info.plist
    rm -rf XCode/EndlessSky-Info.plist.bak
fi

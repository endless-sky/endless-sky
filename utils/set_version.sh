#!/bin/bash

# Sets the application version to the first argument.

# Example:
# $ ./utils/set_version.sh "my cool version"
# $ ./utils/set_version.sh 1.0.0

versionRegex="([0-9]+\.){0,2}[0-9]+"
[[ $1 =~ ^${versionRegex}.*$ ]]
isVersion=$?
if [[ ${isVersion} -eq 0 ]]; then
    replacement="ver. $1"
elif [[ $1 =~ ^[0-9A-Fa-f]+$ ]]; then
    replacement="($1)"
    perl -p -i -e "s/(\"Endless Sky .*)\"/\$1 ${replacement}\"/ig" source/main.cpp
    perl -p -i -e "s/\".*?\" \"(ver\. .*)\" \"/\"$(date '+%d %b %Y')\" \"\$1 ${replacement}\" \"/ig" endless-sky.6
    isVersion=2
else
    replacement=$1
fi

if [[ ${isVersion} -ne 2 ]]; then
    # Update main.cpp
    perl -p -i -e "s/(\"Endless Sky) .+?\"/\$1 ${replacement}\"/ig" source/main.cpp
    # Update endless-sky.6
    perl -p -i -e "s/\".*?\" \"ver\. .+?\"/\"$(date '+%d %b %Y')\" \"${replacement}\"/ig" endless-sky.6
fi

rm -rf source/main.cpp.bak
rm -rf endless-sky.6.bak

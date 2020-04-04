#!/bin/bash

# Adjusts the version strings in main.cpp, credits.txt and EndlessSky-Info.plist for a nightly build.

perl -p -i -e 's/(Endless Sky) [\d.]+/$1 ${{ github.sha }}/ig' source/main.cpp
CREDIT_STRING=$(git log --pretty="format:%nBuilt: $(date -u '+%Y-%m-%d %H:%M') UTC%nLast change by %an: %n %s" -1 | fmt -w 33 -c -s | tr \|\$ _ | tr \" \')
perl -p -i -e "s|version [\d.]+|$CREDIT_STRING|" credits.txt
perl -0777 -p -i -e 's/(CFBundleShortVersionString.*?)[\d.]+/$1sha256.${{ github.sha }}/igs' XCode/EndlessSky-Info.plist
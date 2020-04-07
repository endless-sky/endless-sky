#!/bin/bash

# Updates the version strings in main.cpp and credits.txt

$(readlink -f $0 | xargs dirname)/set_main_cpp_version.sh $(git rev-parse --verify HEAD)
$(readlink -f $0 | xargs dirname)/set_manpage_version.sh $(git rev-parse --verify HEAD)
$(readlink -f $0 | xargs dirname)/set_plist_version.sh $(git rev-list --all --count)
CREDIT_STRING=$(git log --pretty="format:%nBuilt: $(date -u '+%Y-%m-%d %H:%M') UTC%nLast change by %an: %n %s" -1 | fmt -w 33 -c -s | tr \|\$ _ | tr \" \')
perl -p -i -e "s|version [\d.]+|$CREDIT_STRING|" credits.txt
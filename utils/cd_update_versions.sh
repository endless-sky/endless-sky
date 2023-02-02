#!/bin/bash
set -e

# Updates the version strings, build number, and credits.txt

$(dirname $0)/set_version.sh $(git rev-parse --verify HEAD)
CREDIT_STRING=$(git log --pretty="format: (%h)%n%nBuilt: $(date -u '+%Y-%m-%d %H:%M') UTC%nLast change by %an: %n %s" -1 | fmt -w 33 -c -s | tr \|\$ _ | tr \" \')
perl -p -i -e "s|(version [\d.]+(-\w*)?)|\$1$CREDIT_STRING|" credits.txt
rm -rf credits.txt.bak

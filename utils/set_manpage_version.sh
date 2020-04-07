#!/bin/bash

# Sets the application version in endless-sky.6 to the first argument.
# Example: 
# $ ./utils/set_manpage_version.sh "my cool version"

perl -p -i -e "s/\".*?\" \"ver\. [\d.]+\"/\"$(date '+%d %b %Y')\" \"ver. $1\"/ig" endless-sky.6
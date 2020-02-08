#!/bin/bash

# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)

# Go to toplevel ES directory
cd ${HERE}
cd ..

ESTOP=$(pwd)

echo "script-dir: ${HERE}"
echo "es-top-dir: ${ESTOP}"


# Checkout helper script for adding Xcode/project files if they are missing
#git clone https://github.com/zackslash/Xcode-Proj-Adder.git

NUM_ADDED=0
for FILE in $(ls -1 source)
do
	echo "Checking ${FILE}"
	# Check if the file is already in the XCode project
	cat EndlessSky.xcodeproj/project.pbxproj | grep "$FILE"
	if [ $? -ne 0 ] && [ "$FILE" != "WinApp.rc" ]
	then
		echo "File $FILE is missing from XCode-project"
		NUM_ADDED=$(( NUM_ADDED + 1 ))
		#./Xcode-Proj-Adder/bin/XcodeProjAdder \
		#	-XCP ${ESTOP}/EndlessSky.xcodeproj/project.pbxproj \
		#	-SSCV "../source/${FILE}"
	fi
done

if [ ${NUM_ADDED} -gt 0 ]
then
	echo "You should add ${NUM_ADDED} files"
	echo "The Xcode project file now contains:"
	echo ""
	cat EndlessSky.xcodeproj/project.pbxproj
	echo ""
fi

#!/bin/bash

# Script that checks if the XCode (MacOS) project is complete.
# If the XCode project is incomplete, then this script also uses
# mod-pbxproj (https://github.com/kronenthaler/mod-pbxproj) to
# suggest fixes/additions to make the XCode project complete.
#
# mod-pbxproj should already have been installed (through PIP) before
# running this script.


# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)

# Go to toplevel ES directory
cd ${HERE}
cd ..

ESTOP=$(pwd)
XPROJECT=${ESTOP}/EndlessSky.xcodeproj

echo "script-dir: ${HERE}"
echo "es-top-dir: ${ESTOP}"

NUM_ADDED=0
for FILE in $(find source -type f | sed s,^source/,, | sort)
do
	# Check if the file is already in the XCode project
	grep "$FILE" ${XPROJECT}/project.pbxproj > /dev/null
	if [ $? -ne 0 ] && [ "$FILE" != "WinApp.rc" ]
	then
		echo "File $FILE is missing from XCode-project, trying to add the file."
		NO_BUILD=""
		if [ "${FILE: -2}" == ".h" ]
		then
			# ES Xcode projects don't use build sections for header files.
			echo "Not adding build section for header file"
			NO_BUILD="--no-create-build-files"
		fi
		echo "Version:"
		python3 -m pbxproj --version
		echo ""
		echo "Help:"
		python3 -m pbxproj --help
		echo ""
		echo "Help (file):"
		python3 -m pbxproj file --help
		echo ""
		python3 -m pbxproj file ${XPROJECT} "source/${FILE}" --tree "<group>" --parent "source" --target "EndlessSky" ${NO_BUILD}
		echo "Project to add file to XCode project ran with result $?"
		# Check if the requested file was added
		grep "$FILE" ${XPROJECT}/project.pbxproj > /dev/null
		if [ $? -ne 0 ]
		then
			echo "Error: file ${FILE} not added to XCode project"
			echo "Git status:"
			git status
			echo ""
			echo "Diff (between project and checked-in version)"
			git diff ${XPROJECT}/project.pbxproj
			exit 1
		fi
		NUM_ADDED=$(( NUM_ADDED + 1 ))
	fi
done

if [ ${NUM_ADDED} -gt 0 ]
then
	echo "You should add ${NUM_ADDED} files"
	echo "Example of the diff to apply:"
	echo ""
	git diff ${XPROJECT}/project.pbxproj
	echo ""
	exit 1
fi
exit 0

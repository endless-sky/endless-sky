#!/bin/bash

# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)

# Go to toplevel ES directory
cd ${HERE}
cd ..

ESTOP=$(pwd)

echo "script-dir: ${HERE}"
echo "es-top-dir: ${ESTOP}"

NUM_ADDED=0
for FILE in $(ls -1 source)
do
	echo "Checking ${FILE}"
	# Check if the file is already in the XCode project
	cat EndlessSky.xcodeproj/project.pbxproj | grep "$FILE" > /dev/null
	if [ $? -ne 0 ] && [ "$FILE" != "WinApp.rc" ]
	then
		echo "File $FILE is missing from XCode-project"
		if [ ! -d Xcode-Proj-Adder ]
		then
			echo "Cloning Xcode-Proj-Adder project"
			git clone https://github.com/zackslash/Xcode-Proj-Adder.git
			if [ $? -ne 0 ]
			then
				echo "Error: Cloning failed"
				exit 1
			fi
			if [ ! -f ./Xcode-Proj-Adder/bin/XcodeProjAdder ]
			then
				echo "Error: Xcode-adder missing"
				exit 1
			fi
		fi
		# Run XcodeProjAdder and print exit-code.
		./Xcode-Proj-Adder/bin/XcodeProjAdder \
			-XCP ${ESTOP}/EndlessSky.xcodeproj/project.pbxproj \
			-SCSV "../source/${FILE}"
		echo "XcodeProjAdder ran with result $0"
		# Check if the requested file was added
		cat EndlessSky.xcodeproj/project.pbxproj | grep "$FILE" > /dev/null
		if [ $? -eq 0 ]
		then
			echo "Error: file ${FILE} not added to XCode project"
			exit 1
		fi
		NUM_ADDED=$(( NUM_ADDED + 1 ))
	fi
done

if [ ${NUM_ADDED} -gt 0 ]
then
	echo "You should add ${NUM_ADDED} files"
	echo "An example of the project file after adding the missing files:"
	echo ""
	cat EndlessSky.xcodeproj/project.pbxproj
	echo ""
fi

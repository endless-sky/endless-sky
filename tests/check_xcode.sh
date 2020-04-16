#!/bin/bash

# Script that checks if the XCode (MacOS) project is complete.
# If the XCode project is incomplete, then this script also uses
# mod-pbxproj (https://github.com/kronenthaler/mod-pbxproj) to
# suggest fixes/additions to make the XCode project complete.
#
# mod-pbxproj should already have been installed (through PIP) before
# running this script.

has_file() {
	fgrep -q "path = source/${1};" ${XPROJECT}/project.pbxproj
}

has_encoded_file() {
	fgrep "path = source/${1};" ${XPROJECT}/project.pbxproj | fgrep -q "fileEncoding = 4;"
}

add_new() {
	local OPTS='--tree <group> --parent source'
	echo -n "'${1}': "
	python3 -m pbxproj file "${XPROJECT}" "source/${1}" ${OPTS} --target EndlessSky ${2}
	# Make sure the file was added with Unicode encoding (fileEncoding = 4;)
	if ! has_encoded_file ${1}; then
		local GOOD_STR="isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c"
		local BAD_STR="isa = PBXFileReference; lastKnownFileType = sourcecode.c"
		local BACKUP=${XPROJECT}/project.pbxproj.bak
		sed -i.bak s/"${BAD_STR}"/"${GOOD_STR}"/ ${XPROJECT}/project.pbxproj
		if [ $? -ne 0 ]; then
			echo "Using sed to enforce unicode encoding failed! Restoring backup XCode project"
			cp "${BACKUP}" ${XPROJECT}/project.pbxproj
		fi
		rm -f "${BACKUP}"
	fi
}

move_existing() {
	# The simplest method to "move" a file in XCode is to update the referred file path. These lines look like
	# A96862DB1AE6FD0A004FE1FE /* BankPanel.cpp */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = BankPanel.cpp; path = source/BankPanel.cpp; sourceTree = "<group>"; };
	local basename=$(basename ${1})
	local BACKUP=${XPROJECT}/project.pbxproj.bak
	# Since the normal sed delimiter / is used in file paths, we will use _ as the delimiter:
	sed -i.bak "\_path = source/${basename};_ s_/${basename}_/${1}_" ${XPROJECT}/project.pbxproj
	if [ $? -ne 0 ]; then
		echo "Using sed to move '${basename}' to '${1}' failed! Restoring backup XCode project"
		cp "${BACKUP}" ${XPROJECT}/project.pbxproj
	fi
	rm -f "${BACKUP}"
}

# Determine path of the current script, and go to the ES project root.
HERE=$(cd `dirname $0` && pwd)
cd ${HERE}/..

ESTOP=$(pwd)
XPROJECT=${ESTOP}/EndlessSky.xcodeproj

ADDED=()
RESULT=0
for FILE in $(find source -type f | sed s,^source/,, | sort)
do
	# Check if the file is already in the XCode project
	if [[ "${FILE}" != "WinApp.rc" ]] && ! has_file ${FILE}; then
		# If this file is present, but under a different path, we should update the paths.
		BASENAME=$(basename "${FILE}")
		if has_file ${BASENAME}; then
			echo "'${FILE}': updating path."
			move_existing ${FILE}
		else
			NO_BUILD=""
			if [ "${FILE: -2}" == ".h" ]; then
				# ES's Xcode project doesn't use build sections for header files.
				NO_BUILD="--no-create-build-files"
			fi
			add_new ${FILE} ${NO_BUILD}
		fi
		
		# If the addition or rename was unsuccessful, flag it.
		if has_file ${FILE}; then
			ADDED+=(${FILE})
		else
			echo -e "\033[0;31mFailed to add '${FILE}'\033[0m"
			RESULT=1
		fi
	fi
done

if [ ${#ADDED[@]} -gt 0 ]; then
	if [ -n "${GITHUB_ACTION}" ]; then
		echo "Generating patch file artifact for use with 'git apply'"
		git diff -p ${XPROJECT}/project.pbxproj > "${ESTOP}/xcode-project.patch"
	else
		# This script was executed locally, so the user can simply stage and commit the changes if desired.
		echo "Updated XCode project"
	fi
	# If some files needed an update, but could not be automatically updated, let the user know.
	if [ ${RESULT} -ne 0 ]; then
		echo -e "\033[0;31m(Some files require manual manipulation.)\033[0m"
	fi
	# Also set the result to non-zero here (since the project is incomplete)
	RESULT=1
elif [ ${RESULT} -eq 0 ]; then
	echo "No files added to XCode project"
fi
exit ${RESULT}

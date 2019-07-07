#!/bin/bash

# check_coding_style.sh
# Copyright (c) 2019 by Peter van der Meer
#
# Endless Sky is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
# Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.



# This is a checker for coding style issues specifically aimed at following the
# coding style as published on:
# http://endless-sky.github.io/styleguide/styleguide.xml
#
# This checker performs heuristic pattern matching, so there is a small risk that
# constructs will be rejected that are actually allowed. The white-list should be
# used to deal with such false positives and can also be used for allowed deviations.


# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)
SOURCES="${HERE}/../source"

# Desired header, adapted for automatic parsing
DESIRED_HEADER="/* FILENAME
Copyright (c) 2xxx by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/"

function die()
{
	echo "ERROR: $1"
	exit 1
}



# This is a white-list of allowed exceptions. For now just included in the script;
# might want to make this a separate text-file later.
function filter_exceptions()
{
	grep -v \
		-e "WinApp.rc" \
		-e "Files.cpp:		struct stat buf;" \
		-e "Files.cpp:	struct _stat buf;" \
		-e "Files.cpp:	struct stat buf;" \
		-e "ImageBuffer.cpp:		struct jpeg_error_mgr jerr;" \
		-e "Point.h:		struct {" \
		-e "gl_header.h: #include <OpenGL/GL3.h>" \
		-e "gl_header.h: #include <GL/glew.h>"
}



function report_issue()
{
	ERROR_MESSAGE="$1"
	# Just forward pipe-in towards the filter
	filter_exceptions | \
	while read ISSUE; do
		# Print the error-message once
		if [ "${ERROR_MESSAGE}" != "" ]; then
			echo ""
			echo "******************************************************"
			echo "${ERROR_MESSAGE}"
			echo "******************************************************"
			ERROR_MESSAGE=""
		fi
		echo "${ISSUE}"
	done
}



function filter_non_matched_class()
{
	while read MATCH_LINE; do
		FILE_NAME=$(echo "${MATCH_LINE}" | cut -d\: -f1 | sed "s/\.h$//")
		CLASS_NAME=$(echo "${MATCH_LINE}" | cut -d\: -f2 | sed "s/^class //")
		if [ "${FILE_NAME}" != "${CLASS_NAME}" ]; then
			echo "${MATCH_LINE}"
		fi
	done
}



# Helper function to check the sort order of all includes
function check_includes_sort_order()
{
	# List of all files in the codebase with their includes. (file:include)
	# The filenames are sorted, but the include statements are taken in the order in which they are in the file
	ALL_INCL=$(find -type f \( -name "*.h" -or -name "*.cpp" \) | grep "[A-Za-z0-9_.]" | sort | sed "s,./,," | xargs grep --with-filename "#include")

	# Now we prepare a list that we sort according to the style guide (by using additional ordering keys). (file:sortkey:include)
	# Add sort-key 4 for all system includes
	SORT_KEY=$(echo "${ALL_INCL}" | sed "s/:#include </:4:#include </")

	# Get all base classes for classes. Not fully exact, but non-exact matches will be filtered out because there
	# is no matching class in the codebase.
	CLASS_BASES=$(grep "^class" *.h | grep -e "public" -e "private" | grep -v ";$" | sed "s/ {//" |\
		sed "s,std::[A-Za-z0-9_]*, ,g" | sed "s,[<>,\*], ,g" |\
		sed -e "s/class [A-Za-z0-9]*[ :]*/ /" -e "s/public//g" -e "s/private//g" | sed "s/[ ]\+/ /g")

	# Add sort-key 1 for classes/h-files that include it's base class
	IFS_OLD=${IFS}
	IFS="
	"
	for CLASS_BASE in ${CLASS_BASES}; do
		HF=$(echo "${CLASS_BASE}" | cut -d\: -f1)
		BASES=$(echo "${CLASS_BASE}" | cut -d\: -f2)
		IFS=" "
		for BASE in ${BASES}; do
			SORT_KEY=$(echo "${SORT_KEY}" | sed "s/\(^${HF}\):\(#include \"${BASE}.h\"\)/\1:1:\2/")
		done
	done
	IFS=${IFS_OLD}

	# Loop over all header files in the codebase
	# Add sort-key 1 for cpp-files that include its matching header file
	# Add sort-key 2 for regular includes from the codebase
	for CB_HF in $(find *.h -type f | sed "s,^./,,"); do
		CB_CPPF=$(echo "${CB_HF}" | sed "s/\.h$/.cpp/")
		SORT_KEY=$(echo "${SORT_KEY}" |\
			sed "s/\(^${CB_CPPF}\):\(#include \"${CB_HF}\"\)/\1:1:\2/" |\
			sed "s/\([hp]\):\(#include \"${CB_HF}\"\)/\1:2:\2/")
	done

	# Add sort-key 3 for non-standard third-party libaries.
	# And re-sort the full list, including the sort-keys.
	# And then remove the sort keys
	SORT_KEY_EXPECTED=$(echo "${SORT_KEY}" | sed "s/\(\.[hc]\):\(#include \"\)/\1:3:\2/" | sort)
	SORT_KEY=$(echo "${SORT_KEY_EXPECTED}" | sed "s/:[0-9]:/:/")

	# The list sorted according to the Style guide keys should be in the same order as the original list above.
	# Report all entries that are out of order by comparing the lists.
	while [ $(echo "${ALL_INCL}" | wc -l) -gt 1 ]; do
		if [ "$(echo "${ALL_INCL}" | head -n 1)" == "$(echo "${SORT_KEY}" | head -n 1)" ]; then
			# Entries match, move to the next
			ALL_INCL=$(echo "${ALL_INCL}" | tail -n +2)
			SORT_KEY=$(echo "${SORT_KEY}" | tail -n +2)
		else
			# Mismatch in entries. Report the first include
			MISMATCH=$(echo "${ALL_INCL}" | head -n 1)
			echo "${MISMATCH}: Include order not according to style guide"
			MISFILE=$(echo "${MISMATCH}" | cut -d: -f1)
			echo "${SORT_KEY_EXPECTED}" | grep "^${MISFILE}" | sed "s/^${MISFILE}:/-Expected /"
			# Make sure we always move to the next element
			ALL_INCL=$(echo "${ALL_INCL}" | tail -n +2)
			# Remove the mis-matching file from both lists
			ALL_INCL=$(echo "${ALL_INCL}" | grep -v "^${MISFILE}:")
			SORT_KEY=$(echo "${SORT_KEY}" | grep -v "^${MISFILE}:")
		fi
	done
}



# Go to the sources directory
cd "${SOURCES}" || die "Sources directory not found"



# Files section
grep -P -e "\r\n$" * |\
	report_issue "Files: All files should have Unix-style line endings."

grep -P -n "[^\x00-\x7F]" * |\
	report_issue "Files: All files should be plain ASCII."

find -type f | grep -v -e ".h$" -e ".cpp$" | sed "s,^./,," |\
	report_issue "Files: All C++ files should use \".h\" and \".cpp\" for their extensions."

grep "^class" *.h | grep -v -e ";$" | cut -d\: -f1,2 | sed "s,[{].*$,," | sed "s, *$,," | filter_non_matched_class |\
	report_issue "Files: Each class \"MyClass\" should have its own .h and .cpp files, and they should be named \"MyClass.h\" and \"MyClass.cpp\""

for FILE in *; do
	HEADER=$(head -n 11 ${FILE})
	HEADER_CHECK=$(echo "${HEADER}" | sed "s,^/\* ${FILE}$,/* FILENAME," |\
		sed "s,^Copyright (c) 2[0-9]* by [A-Za-z0-9 ]*,Copyright (c) 2xxx by Michael Zahniser,")
	if [ "${HEADER_CHECK}" != "${DESIRED_HEADER}" ]; then
		echo "${FILE}: mismatch in header"
	fi
done |\
	report_issue "Each file should begin with a header which specifies the file name, copyright, and licensing information"

for FILE in *.h; do
	FILE_GUARD_NAME=$(echo "${FILE}" | sed "s/\([a-z]\)\([A-Z]\)/\1_\2/g" | sed "s,\.h,_H_,")
	FILE_GUARD_NAME=$(echo "${FILE_GUARD_NAME^^}")
	# By also grepping for includes we check that the ifndef and defines are before includes.
	GUARDS=$(cat ${FILE} | grep -e "#ifndef" -e "#define" -e "#include" )
	GUARD_IFNDEF=$(echo "${GUARDS}" | head -n 1)
	GUARD_DEFINE=$(echo "${GUARDS}" | head -n 2 | tail -n 1)
	if [ "${GUARD_IFNDEF}" != "#ifndef ${FILE_GUARD_NAME}" ]; then
		echo "${FILE}: ${GUARD_IFNDEF}"
	fi
	if [ "${GUARD_DEFINE}" != "#define ${FILE_GUARD_NAME}" ]; then
		echo "${FILE}: ${GUARD_DEFINE}"
	fi
done |\
	report_issue "All header files should have #define guards, and they should be in the format \"#ifndef MY_CLASS_H_\"."

for FILE in *.cpp; do
	grep --with-filename -e "#include " -e "using namespace std;" "${FILE}" | tail -n 1 | sed "/${FILE}:using namespace std;/d"
done |\
	report_issue "Each .cpp file should put \"using namespace std;\" immediately after the #includes."

check_includes_sort_order |\
	report_issue "Order of #includes in a .h or .c file"



# Formatting section
grep "^  " * |\
	report_issue "Use tabs instead of spaces so that the code can be edited in any editor easily."

grep -e "if (" -e "while (" -e "for (" * | grep -v -e ".*//.*(.*" | grep -v "<<" |\
	report_issue "Do not put a space before or after the parentheses for functions and control statements."

grep -e ",[^ \"'0\`\\]" * |\
	report_issue "Put a space after commas."

# Classes section
grep "class [^A-Z]" * | grep -v "//" | grep -v "template <class" |\
	report_issue "Class names should be CapitalizedCamelCase. Do not use Hungarian notation."

grep -e "[[:space:]]struct[[:space:]]" * |\
	report_issue "Do not use the \"struct\" keyword; just define a class with public data members instead."



#C++11 section
grep "NULL" * |\
	report_issue "Use \"nullptr\" for pointers rather than \"NULL\" or \"0\"."


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
Copyright 2xxx by NAME

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
		-e "Files.cpp:		struct stat buf;" \
		-e "Files.cpp:	struct _stat buf;" \
		-e "Files.cpp:	struct stat buf;" \
		-e "ImageBuffer.cpp:		struct jpeg_error_mgr jerr;" \
		-e "Point.h:		struct {" \
		-e "gl_header.h: #include <OpenGL/GL3.h>" \
		-e "gl_header.h: #include <GL/glew.h>" \
		-e "main.cpp: mismatch in header"
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



# Go to the sources directory
cd "${SOURCES}" || die "Sources directory not found"



# Files section
grep -P -n "[^\x00-\x7F]" * |\
	report_issue "Files: All files should be plain ASCII."

find -type f | grep -v -e ".h$" -e ".cpp$" -e "WinApp.rc" | sed "s,^./,," |\
	report_issue "Files: All C++ files should use \".h\" and \".cpp\" for their extensions."

grep "^class" *.h | tr -d '\r' | grep -v -e ";$" | cut -d\: -f1,2 | sed "s,[{].*$,," | sed "s, *$,," | filter_non_matched_class |\
	report_issue "Files: Each class \"MyClass\" should have its own .h and .cpp files, and they should be named \"MyClass.h\" and \"MyClass.cpp\""

for FILE in *.{h,cpp}; do
	HEADER=$(head -n 11 ${FILE})
	HEADER_CHECK=$(echo "${HEADER}" | sed "s,^/\* ${FILE}$,/* FILENAME," |\
		sed "s/[ ]*(c)[ ]*/ /" | sed "s,^[Cc]opyright 2[0-9]* by [^\n\r]*,Copyright 2xxx by NAME,")
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

# Check if "using namespace std;" is after the includes
for FILE in *.cpp; do
	grep --with-filename -e "#include " -e "using namespace std;" "${FILE}" | head -n -1 | grep "using namespace std;"
done |\
	report_issue "Each .cpp file should put \"using namespace std;\" immediately after the #includes."


# Formatting section
grep "^  " * |\
	report_issue "Use tabs instead of spaces so that the code can be edited in any editor easily."

grep -e "if (" -e "while (" -e "for (" * | grep -v -e ".*//.*(.*" | grep -v "<<" |\
	report_issue "Do not put a space before or after the parentheses for functions and control statements."

grep "," * | tr -d '\r' | grep -e ",[^ \"'0\`\\]" |\
	report_issue "Put a space after commas."

# Classes section
grep "class [^A-Z]" * | grep -v "//" | grep -v "template <class" |\
	report_issue "Class names should be CapitalizedCamelCase. Do not use Hungarian notation."

grep -e "[[:space:]]struct[[:space:]]" * |\
	report_issue "Do not use the \"struct\" keyword; just define a class with public data members instead."



#C++11 section
grep "NULL" * |\
	report_issue "Use \"nullptr\" for pointers rather than \"NULL\" or \"0\"."

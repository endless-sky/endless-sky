#!/usr/bin/python
# check_content_style.py
# Copyright (c) 2022 by tibetiroka
#
# Endless Sky is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
# Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

import regex as re
import glob

# Script that checks for common text content formatting pitfalls not covered by spellcheck or other tests.

# Regex-based style checks for check_regexes()
regexes = {re.compile(regex): description for regex, description in {
	# Matches any space that is preceded only by tabs or '"' or '`' characters,
	# except if it immediately follows a '"' or '`' character, as used in 'word'.
	"^[\\t`\"]*(?<![`\"]) ": "indentations should use tabs only",
	# Matches multiple space characters, unless they are followed by a '#', or if the text is a tooltip.
	"(?<!^tip .*)(  $|  [^ #])": "multiple consecutive space characters",
	# Matches any whitespace character at the end of the string.
	"[^\\s]\\s+$": "trailing whitespace character",
	# Matches any backticks at the end of the line that is
	# preceded by the ending of a sentence ('?', '!', '.', ';', ':', ')', '"', ''', '>') and a whitespace.
	# The same check fails if used on quotation marks, due to the irregular nature of 'word'.
	"[?!.;:\\)\"'>] `$": "unnecessary space before closing backtick"
}.items()}


# Checks the format of all txt files.
def check_content_style():
	error_count = 0
	files = glob.glob('data/**/*.txt', recursive=True)
	files.sort()
	for file in files:

		f = open(file, "r")
		lines = f.readlines()
		lines = [line.removesuffix('\n') for line in lines]

		error_list = []
		if file != "data/tooltips.txt":
			error_list += check_copyright(lines)
		error_list += check_ascii(lines)
		error_list += check_regexes(lines)
		error_list += check_indents(lines)

		# Making sure errors are printed in order of their line numbers
		error_list.sort(key=lambda error: error[0])
		for (line, reason) in error_list:
			print("Formatting error in " + file + " line " + str(line) + ": " + reason)
		error_count += len(error_list)
		error_list.clear()
	return error_count


# Checks the copyright header of the files. Unlike C++ source files, these don't contain the file's name in the header.
# The tooltips.txt file should be excluded from this check, as it has no copyright header.
# Parameters:
# lines: the lines of the file
# Returns: a list of tuples representing the location and the description of the formatting issues
def check_copyright(lines):
	error_list = []
	# The mandatory first line of a copyright header; might be repeated
	first = "^# Copyright \\(c\\) \\d{4}(?:(?:-|, )\\d{4})? by .+$"
	# Optional line following the first one; might be repeated
	second = "^# Based on earlier text copyright \\(c\\) \\d{4}(?:(?:-|, )\\d{4})? by .+$"
	# The final segment of the copyright header
	third = """\
	#
	# Endless Sky is free software: you can redistribute it and/or modify it under the
	# terms of the GNU General Public License as published by the Free Software
	# Foundation, either version 3 of the License, or (at your option) any later version.
	#
	# Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
	# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	# PARTICULAR PURPOSE. See the GNU General Public License for more details.
	#
	# You should have received a copy of the GNU General Public License along with
	# this program. If not, see <https://www.gnu.org/licenses/>.
	\
	""".splitlines()
	text_index = 0
	for line in lines:
		if re.search(first, line):
			text_index += 1
		else:
			break
	if text_index == 0:
		error_list.append((1, "incorrect copyright header"))
		return error_list
	for line in lines[text_index:]:
		if re.search(second, line):
			text_index += 1
		else:
			break
	if len(lines) - text_index < len(third):
		error_list.append((len(third) - 1, "incomplete copyright header"))
	else:
		for index, (line, standard) in enumerate(zip(lines[text_index:], third)):
			standard = standard.strip()
			if standard != line:
				error_list.append((text_index + index + 1, "incorrect copyright header"))
				break
	return error_list


# Checks whether the file contains any non-ASCII characters. Extended ASCII codes are not accepted.
# Parameters:
# lines: the lines of the file
# Returns: a list of tuples representing the location and the description of the formatting issues
def check_ascii(lines):
	error_list = []
	for index, line in enumerate(lines):
		for char in line:
			if ord(char) < 0 or ord(char) > 127:
				error_list.append((index + 1, "files should be plain ASCII"))
	return error_list


# Performs the regex-based formatting checks.
# Parameters:
# lines: the lines of the file
# Returns: a list of tuples representing the location and the description of the formatting issues
def check_regexes(lines):
	error_list = []
	for index, line in enumerate(lines):
		for regex, description in regexes.items():
			if re.search(regex, line):
				error_list.append((index + 1, description))
	return error_list


# Checks that each line is properly indented based on the indentation of surrounding lines.
# Parameters:
# lines: the lines of the file
# Returns: a list of tuples representing the location and the description of the formatting issues
def check_indents(lines):
	error_list = []
	previous_indent = 0
	for index, line in enumerate(lines):
		indent = count_indent(line)
		# Checking non-empty lines
		if line and not line.isspace():
			if indent - previous_indent > 1:
				error_list.append((index + 1, "do not add more than a single level of indentation per line"))
			previous_indent = indent
	return error_list


# Counts the number of tabs this line of text is indented with.
# Parameters:
# text: the text to count indents in
# Returns: the number of leading tabulators
def count_indent(text):
	count = 0
	for char in text:
		if char == '\t':
			count += 1
		else:
			break
	return count


# Generates and stores an error message. The error is later displayed in check_content_style().
def print_error(file, line, reason):
	error_list.append((line, "Formatting error in " + file + " line " + str(line) + ": " + reason))


if __name__ == '__main__':
	error_count = check_content_style()
	if error_count > 0:
		text = "Found " + str(error_count) + " formatting " + ("issue" if error_count == 1 else "issues") + "."
		print(text)
		exit(1)
	else:
		print("No formatting issues found.")
	exit(0)

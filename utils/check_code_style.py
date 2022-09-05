#!/usr/bin/python
# check_coding_style.py
# Copyright (c) 2022 by tibetiroka
#
# Endless Sky is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
# Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License for more details.

import os

import regex as re
import glob

# Script that checks for common code formatting pitfalls not covered by clang-format or other tests.
# The formatting rules are generally based on the guide found at http://endless-sky.github.io/styleguide/styleguide.xml
# Unit tests mandate the existence of several exceptions to these rules.
#
# This checker uses regular expressions. For the sake of simplicity, these expressions represent a rather loose
# interpretation of the rules.

# String version of the regexes for easy editing
# List of the standard operators that are checked
std_op = "\\+/\\*<>&%=\\|!:\\-"
# Dict of patterns for selection potential formatting issues in full lines.
# These lines don't contain the contents of strings, chars or comments.
# The dict also contains the error description for the patterns.
line_include = {
	# Matches any '{' following an 'if', 'else if', 'for', 'while' or 'switch' statement.
	"^(?:(}\\selse\\s)?if|else|for|switch|catch|while)\\W*{$": "'{' should be on new line",
	# Matches any '{' not preceded by a whitespace or '(', except when the '{' is closed on the same line.
	"(?<!^(struct|inline).*)[^\\s(]+{(?!.*})": "missing whitespace before '{'",
	# Matches any parenthesis preceded by a whitespace,
	# except if the whitespace follows a semicolon,
	# or follows an all-caps method name
	"(?![A-Z]+)^.*[^;]\\s\\)": "extra whitespace before closing parenthesis",
	# Matches any 'if', 'else if', 'else', 'for', 'catch', 'try', 'do', or 'switch' statements
	# where the statement is not at the beginning of the line.
	"(?<!^inline\\s.*)[^\\w0-9]((?<!else\\s)if|else|else\\sif|switch|for|catch|try|do)(\\s{|\\()": "statement should begin on new line",
	# Matches any semicolons not at the end of line, unless they are inside 'for' statements
	";[^\\)}]+$": "semicolon should terminate line",
	# Matches any whitespaces at the end of a line
	"\\s+$": "trailing whitespace at end of line",
}
# Dict of patterns for selecting potential formatting issues in a full segment.
# (a segment is a part of a line that is between any strings, chars or comments)
# Also contains the error description for the patterns.
segment_include = {
	# Matches at least 2 whitespace characters following a non-whitespace character unless the entire line
	# is made up of commas, numbers or variable names.
	# This is necessary to avoid flagging array-declaration tables that have custom indentation for readability.
	"(?![\\s\\w\\.,\\d{}-])^.*\\S\\s\\s+.*$": "consecutive whitespace characters",
	# Matches any '(' that has no following whitespace,
	# except if the whitespace is followed by a semicolon,
	# or follows an all-caps method name.
	"(?![A-Z]+)^.*\\(\\s(?!;)": "extra whitespace after opening parenthesis",
	# Matches any 'if', 'for', 'catch' or 'switch' statements where the '(' is preceded by a whitespace.
	"(?:if|switch|for|catch)\\s+\\(": "extra whitespace before '('",
	# Matches any 'try' or 'do' statements that are not followed by a whitespace and a '{'.
	# The missing whitespace is checked in another pattern.
	"^(try|do)$": "'try' or 'do' and '{' should be on the same line",
	# Matches any tabulator characters.
	"\t": "tabulators should only be used for indentation",
	# Matches any commas that are not followed by whitespace characters.
	",\\S": "commas should be followed by whitespaces"
}
# Dict of patterns for selecting potential formatting issues in a single word.
# Also contains the error description for the patterns.
word_include = {
	# Matches any number of operators that have no leading whitespace,
	# except if preceded by '(', '[' or '{'.
	"([^([{\\s" + std_op + "](?<!^.*[^\\w0-9]?operator))[" + std_op + "]+([^.\\)" + std_op + "]|$)(?!\\.\\.\\.)": "missing whitespace before operator",
	# Matches any single '+', '/', '%', '=' operator that has no trailing whitespace.
	"^([^+/%=]?(?<!operator))[+/%=][^+/%=,\\s\\)\\]}]": "missing whitespace after operator",
	# Matches any series of operators ending with '=', '<' or '>' that have no trailing whitespace.
	"^[^<>=:]?[" + std_op + "]*[=<>:][^=<>:,\\s\\)\\]}]": "missing whitespace after operator",
	# Matches any 'dynamic_cast' or 'const_cast' statements
	"^(dynamic|const)_cast<": "C-style typecasts are not to be used",
	# Matches any '(void)' arguments in methods
	"\\(void\\)": "do not use void to denote a function with no arguments"
}

# Patterns for excluding matches (test()#match) of 'include'
match_exclude = [
	# Matches any repeating +, - or : operators, or any ::* or ::& references
	"^.?([+:-])\\1+|::&|::\\*.?$",
	# Matches any matches which have a -> operator surrounded by at most 1 character on either side.
	"^.?->.?$",
	# Matches any matches which have a character followed by '*(' or '&('
	"^\\w[*&]\\($",
	# Matches any exponent-related matches.
	"^e[+-]\\d+$"
]
# Patterns for excluding segments that had matches in $include
segment_exclude = [
	# Matches anything inside '<>'; this is a bit of a hack for getting rid of type-related issues
	"<.*>",
	# Matches any visibility modes; these are followed by ':' marks.
	"^(public|protected|private|default):$"
]

# Precompiled version of the regexes
line_include = {re.compile(regex): description for regex, description in line_include.items()}
segment_include = {re.compile(regex): description for regex, description in segment_include.items()}
word_include = {re.compile(regex): description for regex, description in word_include.items()}
match_exclude = [re.compile(regex) for regex in match_exclude]
segment_exclude = [re.compile(regex) for regex in segment_exclude]

# Precompiled  helper regexes
after_comment = re.compile("[^\\s#]")
whitespace_only = re.compile("^\\s*$")
whitespaces = re.compile("\\s+")

# The number of errors found in all files
errors = 0
# The number of warnings found in all files
warnings = 0
# The list of errors and warnings for the current file
error_list = []


# A class used for buffering error messages.
# text: the text where the error originates from
# file: the path to the file
# line: the current line number
# reason: the reason for the error
class Error(object):

	def __init__(self, text, file, line, reason):
		self.text = text
		self.file = file
		self.line = line
		self.reason = reason
		global errors
		errors += 1
		global error_list
		error_list.append(self)

	# Displays the message for this error
	def print(self):
		print("Formatting error in file", self.file, "line", self.line.__str__() + ":", self.text.replace('\n', ""))
		print("\tReason:", self.reason)


# A class used for buffering error messages.
# text: the text where the warning originates from
# file: the path to the file
# line: the current line number
# reason: the reason for the warning
class Warning(Error):

	def __init__(self, text, file, line, reason):
		Error.__init__(self, text, file, line, reason)

		global errors
		errors -= 1
		global warnings
		warnings += 1

	def print(self):
		print("Warning:", self.reason, "in", self.file, "line", self.line.__str__() + ":", self.text.replace('\n', ""))


# Checks the format of all source files.
def check_code_style():
	files = glob.glob('**/*.cpp', recursive=True) + glob.glob('**/*.h', recursive=True)
	files.sort()
	for file in files:

		f = open(file, "r")
		original_lines = f.readlines()
		original_lines = [line.removesuffix('\n') for line in original_lines]

		check_pre_sanitize(original_lines, file)

		segmented_lines = sanitize(original_lines, file)
		sanitized_lines = ["".join(segments) for segments in segmented_lines]

		check_global_format(sanitized_lines, segmented_lines, original_lines, file)
		check_local_format(sanitized_lines, segmented_lines, file)

		error_list.sort(key=lambda error: error.line)
		for error in error_list:
			error.print()
		error_list.clear()


# Sanitizes the contents of the file by removing the contents of strings and comments.
# Also performs some minimal format checking that cannot be done elsewhere.
# Parameters:
# lines: the original contents of the file, without trailing line separators
# file: the path to the file
# skip_checks: whether to skip checks for formatting errors
def sanitize(lines, file, skip_checks=False):
	is_multiline_comment = False
	is_string = False
	is_char = False
	is_raw_string = False
	is_raw_string_short = False
	line_count = 0
	header_found = False

	line_segments = []

	for line in lines:
		line_count += 1
		segments = []
		is_escaped = False
		# Start index is the beginning of the sequence to be tested
		start_index = 0
		# Looking for parts of the file that are not strings or comments
		for i in range(len(line)):
			char = line[i]
			# Handling character escapes
			if is_escaped:
				is_escaped = False
				continue
			elif char == '\\':
				is_escaped = True
				continue
			# Handling comments
			first_two = line[i:i + 2]
			if is_multiline_comment:
				if first_two == "*/":
					if not skip_checks:
						# Checking for space after comment
						if i > 0 and line[i - 1] != ' ' and line[i - 1] != '\t':
							write_error(line[i - 1:i + 2], file, line_count,
							            "missing space before end of multiline comment")
					# End of comment
					is_multiline_comment = False
					i += 1
					start_index = i + 1
				continue
			if (not is_string) and first_two == "//":
				segments.append(line[start_index:i].rstrip())
				if not skip_checks:
					# Checking for space after comment
					if len(line) > i + 2:
						if re.search(after_comment, line[i + 2:i + 3]):
							write_error(line[i:i + 3], file, line_count,
							            "missing space after beginning of single-line comment")
				break
			elif (not is_string) and first_two == "/*":
				segments.append(line[start_index:i].rstrip())
				is_multiline_comment = True
				if not skip_checks:
					if header_found and not (
							line[i + 1:].count("*/") >= 1 and (line.endswith(")") or line.endswith("{"))):
						write_error(line.lstrip(), file, line_count,
						            "multiline comments should only be used for the copyright header")
					# Checking for space after comment
					if len(line) > i + 2 and line[i + 2] != ' ':
						write_error(line[i:i + 3], file, line_count,
						            "missing space after beginning of multiline comment")
				header_found = True
				continue
			# Checking for strings (both standard and raw literals)
			elif (not is_string) and char == "'":
				if is_char:
					start_index = i
				else:
					segments.append(line[start_index:i + 1])
				is_char = not is_char
			elif is_char:
				continue
			elif char == '"':
				if line[i:i + 4] == "\"\"\"\"":
					if is_raw_string:
						start_index = i + 3
					else:
						segments.append(line[start_index:i + 1])
					is_raw_string = not is_raw_string
					is_string = not is_string
				elif line[i - 1:i + 2] == "R\"(":
					if is_raw_string:
						continue
					if is_raw_string_short:
						continue
					is_raw_string_short = True
					is_string = True
					segments.append(line[start_index:i + 1])
				elif line[i - 1:i + 1] == ")\"" and is_raw_string_short:
					is_raw_string_short = False
					is_string = False
					start_index = i
				else:
					if is_raw_string or is_raw_string_short:
						continue
					if is_string:
						start_index = i
					else:
						segments.append(line[start_index:i + 1])
					is_string = not is_string
		else:
			if (not is_multiline_comment) and (not is_char) and (not is_escaped) and (not is_string):
				segments.append(line[start_index:])
		line_segments.append(segments)
	return line_segments


# Runs checks on the contents of the file before sanitization. Parameters:
# lines: the lines of the file, without the terminating line separators. Contains the contents of strings and comments.
# file: the path to the file
def check_pre_sanitize(lines, file):
	check_copyright(lines, file)
	line_count = 0
	for line in lines:
		line_count += 1
		if len(line) > 120:
			write_error(line, file, line_count, "lines should hard wrap at 120 characters")
		for char in line:
			if ord(char) < 0 or ord(char) > 127:
				write_error(line, file, line_count, "files should be plain ASCII")
				break


# Tests whether the specified file contains any formatting issues. Parameters:
# lines: the lines of the file, without terminating line separators and the contents of strings or comments
# segmented_lines: the segments of each line
# file: the path to the file
def check_local_format(lines, segmented_lines, file):
	line_count = 0
	for line, segments in zip(lines, segmented_lines):
		line_count += 1
		line = line.lstrip()
		if len(segments) > 0:
			segments[0] = segments[0].lstrip()
		check_regex_format(line, segments, line_count, file)


# Tests whether the specified file contains any formatting issues, based on the regex tests. Parameters:
# line: the line to test, without the contents of strings or comments
# segments: the segments of the line
# line_count: the position of the line
# file: the path to the file
def check_regex_format(line, segments, line_count, file):
	for regex, description in line_include.items():
		if check_match(regex, line, line, file, line_count, description):
			return
	for segment in segments:
		# Skip empty
		if re.match(whitespace_only, segment):
			return
		for regex, description in segment_include.items():
			if check_match(regex, segment, segment, file, line_count, description):
				return
		for token in re.split(whitespaces, segment):
			token = token.strip()
			if token != "":
				for regex, description in word_include.items():
					if check_match(regex, token, segment, file, line_count, description):
						return


# Checks if the specified regex matches with the text. Parameters:
# regex: the regex to match
# part: the part of the text to match
# segment: the segment the part belongs to
# file: the path to the file
# line_count: the current line number
# reason: the reason to display if the regex matches the text
def check_match(regex, part, segment, file, line_count, reason):
	pos = re.search(regex, part)
	if pos is not None:
		match = part[pos.start():pos.end()]
		for temp in match_exclude:
			if re.search(temp, match):
				return False
		else:
			for temp in segment_exclude:
				if re.search(temp, segment):
					return False
		write_error(segment, file, line_count, reason)
		return True
	return False


# Checks certain global formatting properties of files, such as their copyright headers. Parameters:
# sanitized_lines: the sanitized contents of the file
# line_segments: the segments of each sanitized line
# original_lines: the contents of the file, without sanitization
# file: the path to the file
def check_global_format(sanitized_lines, line_segments, original_lines, file):
	check_include(sanitized_lines, original_lines, file)


# Checks if the copyright header of the file is correct. Parameters:
# lines: the lines of the file, without the terminating line separators
# file: the path to the file
def check_copyright(lines, file):
	name = file.split("/")[-1]
	copyright_begin = [
		["/* " + name, False],
		["Copyright \\(c\\) \\d{4}(?:(?:-|, )\\d{4})? by .*", True]
	]
	copyright_end = [
		["", False],
		["Endless Sky is free software: you can redistribute it and/or modify it under the", False],
		["terms of the GNU General Public License as published by the Free Software", False],
		["Foundation, either version 3 of the License, or (at your option) any later version.", False],
		["", False],
		["Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY", False],
		["WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A", False],
		["PARTICULAR PURPOSE. See the GNU General Public License for more details.", False],
		["", False],
		["You should have received a copy of the GNU General Public License along with", False],
		["this program. If not, see <https://www.gnu.org/licenses/>.", False],
		["*/", False],
		["", False]
	]
	index = 0
	error_line = -1
	complete = False
	for [copyright, is_regex] in copyright_begin:
		if is_regex:
			if not re.search(copyright, lines[index]):
				error_line = index
				break
		else:
			if copyright != lines[index]:
				error_line = index
				break
		index += 1
	not_found_error_line = index
	if error_line == -1:
		index_begin = index
		while index_begin < len(lines) - len(copyright_end):
			index = index_begin
			for [copyright, is_regex] in copyright_end:
				if is_regex:
					if not re.search(copyright, lines[index]):
						error_line = index
						break
				else:
					if copyright != lines[index]:
						error_line = index
						break
				index += 1
			if error_line == -1:
				complete = True
				break
			index_begin += 1
			error_line = -1
	if error_line != -1:
		write_error(lines[error_line], file, error_line + 1, "invalid or missing copyright header")
	elif not complete:
		write_error(lines[not_found_error_line], file, not_found_error_line + 1,
		            "invalid or incomplete copyright header")


# Checks the import statements at the beginning of the file. Parameters:
# sanitized_lines: the lines of the file, without the line separators and the contents of strings and comments
# original_lines: the lines of the file, without the terminating line separators
# file: the path to the file
def check_include(sanitized_lines, original_lines, file):
	if file == "source/main.cpp":
		return
	# opengl.h is treated as a <> include in most cases
	original_lines = [line if line != "#include \"opengl.h\"" else "#include <opengl.h>" for line in original_lines]

	name = file.split("/")[-1]
	if name.endswith(".cpp"):
		name = name[0:-4] + ".h"

	include_lines = [index for index, line in enumerate(sanitized_lines) if line.startswith("#include ")]
	groups = []
	previous = -2
	for i in include_lines:
		if i == previous + 1:
			groups[-1].append(i)
		else:
			groups.append([i])
		previous = i

	if file.endswith(".cpp") and name[0].isupper():
		if len(groups) == 0:
			write_warning("", file, 0, "missing include statement for own header file")
			return
		elif original_lines[groups[0][0]] != "#include \"" + name + "\"":
			write_warning(original_lines[groups[0][0]], file, groups[0][0], "missing include for own header file")
		if len(groups[0]) > 1:
			write_warning(original_lines[groups[0][1]], file, groups[0][1],
			              "missing empty line after including own header file")
	for group in groups:
		quote = original_lines[group[0]].endswith("\"")
		for index in group:
			if original_lines[index].endswith("\"") != quote:
				write_warning(original_lines[index], file, index, "missing empty line before changing include style")
				break
		group_lines = [original_lines[index] for index in group]
		for i in range(len(group_lines)):
			line = group_lines[i]
			if line.count("/") > 0:
				if quote:
					line = line[0:line.find("\"") + 1] + line[line.rfind("/") + 1:len(line)]
				else:
					line = line[0:line.find("<") + 1] + line[line.rfind("/") + 1:len(line)]
				group_lines[i] = line
		for i in range(len(group) - 1):
			if group_lines[i].lower() > group_lines[i + 1].lower():
				write_warning(group_lines[i], file, group[i] + 1, "includes are not in alphabetical order")


# Displays an error message. Parameters:
# text: the text where the formatting error was found
# file: the path to the file
# line: the current line number
# reason: the reason for the formatting error
def write_error(text, file, line, reason):
	Error(text, file, line, reason)


# Displays a warning message. Parameters:
# text: the text where the warning originates from
# file: the path to the file
# line: the current line number
# reason: the reason for the warning
def write_warning(text, file, line, reason):
	Warning(text, file, line, reason)


if __name__ == '__main__':
	check_code_style()
	if errors > 0:
		text = "Found " + errors.__str__() + " formatting " + ("error" if errors == 1 else "errors")
		if warnings > 0:
			text += " and " + warnings.__str__() + " " + ("warning" if warnings == 1 else "warnings")
		text += "."
		print(text)
		exit(1)
	if warnings == 0:
		print("No formatting errors found.")
	else:
		print(warnings, "warning" if warnings == 1 else "warnings", "found.")
	exit(0)

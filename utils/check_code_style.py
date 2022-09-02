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
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.

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
stdOp = "+/*<>&%=|!:-"
stdOp = "+/\\*<>&%=|!:\\-"
# Dict of patterns for selection potential formatting issues in full lines.
# These lines don't contain the contents of strings, chars or comments.
# The dict also contains the error description for the patterns.
line_include = {
	# Matches any '{' following an 'if', 'else if', 'for' or 'switch' statement.
	"^(?:(}\\selse\\s)?if|for|switch|catch).*{$": "'{' should be on new line",
	# Matches any '{' not preceded by a whitespace or '(', except when the '{' is closed on the same line.
	"(?<!^(struct|inline).*)[^\\s(]+{(?!.*})": "missing whitespace before '{'",
	# Matches any parenthesis preceded by a whitespace,
	# except if the whitespace follows a semicolon,
	# or follows an all-caps method name
	"(?![A-Z]+)^.*[^;]\\s\\)": "extra whitespace before closing parenthesis",
	# Matches any 'if', 'else if', 'for', 'catch', 'try' or 'switch' statements where the statement is not at the beginning of the line.
	"(?<!^(inline|struct).*)[^\\w0-9]((?<!else\\s)if|else\\sif|switch|for|catch|try)(\\s{|\\()": "statement should begin on new line",
	# Matches any semicolons not at the end of line, unless they are inside 'for' statements
	";[^\\)}]+$": "semicolon should terminate line"
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
	# Matches any whitespaces at the end of a line that are preceded by brackets, parentheses or semicolons.
	"[;[{]\\s+$": "trailing whitespace at end of line",
	# Matches any 'if', 'for', 'catch' or 'switch' statements where the '(' is preceded by a whitespace.
	"(?:if|switch|for|catch)\\s+\\(": "extra whitespace before '('",
	# Matches any 'try' statements that are not followed by a whitespace and a '{'.
	# The missing whitespace is checked in another pattern.
	"^try$": "'try' and '{' should be on the same line",
	# Matches any tabulator characters.
	"\t": "tabulators should only be used for indentation"
}
# Dict of patterns for selecting potential formatting issues in a single word.
# Also contains the error description for the patterns.
word_include = {
	# Matches any number of operators that have no leading whitespace,
	# except if preceded by '(', '[' or '{'.
	"[^([{\\s" + stdOp + "][" + stdOp + "]+[^" + stdOp + "]*": "missing whitespace before operator",
	# Matches any single '+', '/', '%', '=' operator that has no trailing whitespace.
	"^[^+/%=]*[+/%=][^+/%=,\\s]": "missing whitespace after operator",
	# Matches any series of operators ending with '=', '<' or '>' that has no trailing whitespace.
	"^[^<>=:]?[" + stdOp + "]*[=<>:][^=<>:,\\s]": "missing whitespace after operator"
}

# Patterns for excluding matches (test()#match) of 'include'
match_exclude = [
	# Matches any repeating +, - or : operators
	"([+:-])\\1+",
	# Matches any matches which have a -> operator following the first character
	"^.->",
	# Matches any matches ending with an operator and a bracket
	"[" + stdOp + "][)\\]}]$",
	# Matches any matches ending in triple dots, fixing an issue with vararg references.
	"\\.\\.\\.$",
	# Matches any exponent-related matches.
	"^e[+-]"
]
# Patterns for excluding segments that had matches in $include
segment_exclude = [
	# Matches anything that begins with an include statement; this prevents flagging '<name>' for spacing issues
	"^#include",
	# Matches anything inside '<>'; this is a bit of a hack for getting type-related issues
	"<.*>",
	# Matches any visibility modes; these are followed by ':' marks.
	"^(public|protected|private|default):$",
	# Matches any segments containing operator declarations, since those suck
	"operator"
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


# Checks the format of all source files.
def check_code_style():
	files = glob.glob('**/*.cpp', recursive=True) + glob.glob('**/*.h', recursive=True)
	for file in files:
		check_code_format(file)
		check_global_format(file)


# Tests a file for syntax formatting errors. Parameters:
# file: The path to the file
def check_code_format(file):
	is_multiline_comment = False
	is_string = False
	is_char = False
	is_raw_string = False
	is_raw_string_short = False
	line_count = 0
	f = open(file, "r")
	lines = f.readlines()
	for line in lines:
		line = line.removesuffix('\n')
		# Checking the width of the line
		if len(line) > 120:
			write_error(line, file, line_count, "lines should hard wrap at 120 characters")
		segments = []
		line = line.lstrip()
		line_count += 1
		is_escaped = False
		# Start index is the beginning of the sequence to be tested
		start_index = 0
		# Looking for parts of the file that are not strings or comments
		for i in range(len(line)):
			# Getting current character
			char = line[i]
			# Checking for non-ASCII characters
			if ord(char) < 0 or ord(char) > 127:
				write_error(line, file, line_count, "files should be plain ASCII")
				break
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
					# Checking for space after comment
					if i > 0 and line[i - 1] != ' ' and line[i - 1] != '  ':
						write_error(line[i - 1:i + 2], file, line_count,
						            "missing space before end of multiline comment")
					# End of comment
					is_multiline_comment = False
					i += 1
					start_index = i + 1
				continue
			if (not is_string) and first_two == "//":
				segments.append(line[start_index:i].rstrip())
				# Checking for space after comment
				if len(line) > i + 2:
					if re.search(after_comment, line[i + 2:i + 3]):
						write_error(line[i:i + 3], file, line_count,
						            "missing space after beginning of single-line comment")
				break
			elif (not is_string) and first_two == "/*":
				segments.append(line[start_index:i].rstrip())
				is_multiline_comment = True
				# Checking for space after comment
				if len(line) > i + 2 and line[i + 2] != ' ':
					write_error(line[i:i + 3], file, line_count, "missing space after beginning of multiline comment")
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
				elif line[i - 1:i + 1] == ")\"" and is_raw_string_short:
					if is_raw_string:
						continue
					is_raw_string_short = False
					is_string = False
					segments.append(line[start_index:i + 1])
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
			test(segments, file, line_count)


# Tests whether the specified segments contain any formatting issues. Parameters:
# segments: the list of segments on the line
# file: the path to the file
# line: the current line number
def test(segments, file, line):
	line_text = "".join(segments)
	for regex, description in line_include.items():
		if check_match(regex, line_text, line_text, file, line, description):
			return
	for segment in segments:
		# Skip empty
		if re.match(whitespace_only, segment):
			return
		for regex, description in segment_include.items():
			if check_match(regex, segment, segment, file, line, description):
				return
		for token in re.split(whitespaces, segment):
			token = token.strip()
			if token != "":
				for regex, description in word_include.items():
					if check_match(regex, token, segment, file, line, description):
						return


# Checks if the specified regex matches with the text. Parameters:
# regex: the regex to match
# part: the part of the text to match
# segment: the segment the part belongs to
# file: the path to the file
# line: the current line number
# reason: the reason to display if the regex matches the text
def check_match(regex, part, segment, file, line, reason):
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
		write_error(segment, file, line, reason)
		return True
	return False


# Checks certain global formatting properties of files, such as their copyright headers. Parameters:
# file: the path to the file
def check_global_format(file):
	check_copyright(file)


# Checks if the copyright header of the file is correct. Parameters:
# file: the path to the file
def check_copyright(file):
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
		["PARTICULAR PURPOSE.  See the GNU General Public License for more details.", False],
		["*/", False]
	]
	lines = open(file, "r").readlines()
	lines = [line.removesuffix('\n') for line in lines]
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
		write_error(lines[len(lines) - 1], file, len(lines) - 1, "incomplete copyright header")


# Displays an error message. Parameters:
# text: the text where the formatting error was found
# file: the path to the file
# line: the current line number
# reason: the reason for the formatting error
def write_error(text, file, line, reason):
	print("Formatting error in file", file, "line", line.__str__() + ":", text.replace('\n', ""))
	print("\tReason:", reason)
	global errors
	errors += 1


if __name__ == '__main__':
	check_code_style()
	if errors > 0:
		print("Found", errors, "formatting", "error." if errors == 1 else "errors.")
		exit(1)
	print("No formatting errors found.")
	exit(0)

#!/usr/bin/python
# check_code_style.py
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

import glob
import sys

import regex as re

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
line_include = {re.compile(regex): description for regex, description in {
	# Matches any '{' following an 'if', 'else if', 'for', 'while' or 'switch' statement.
	"^(else\\sif|if|else|for|switch|catch|while)\\s?\\(.*{$": "'{' should be on new line",
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
	# Matches any number of operators that have no leading whitespace,
	# except if preceded by '(', '[' or '{', or inside a 'case' constant expression.
	"(?<!^case\\s.*)([^([{\\s" + std_op + "](?<!^.*[^\\w0-9]?operator))[" + std_op + "]+(?<!(->|::|\\.)\\*)([^.\\)" + std_op + "]|$)(?!\\.\\.\\.)": "missing whitespace before operator"
}.items()}
# Dict of patterns for selecting potential formatting issues in a full segment.
# (a segment is a part of a line that is between any strings, chars or comments)
# Also contains the error description for the patterns.
segment_include = {re.compile(regex): description for regex, description in {
	# Matches at least 2 whitespace characters following a non-whitespace character unless the entire line
	# is made up of commas, numbers or variable names.
	# This is necessary to avoid flagging array-declaration tables that have custom indentation for readability.
	"(?!^[\\s\\w\\.,\\d{}+\\-]*$)^.*\\S\\s\\s+.*$": "consecutive whitespace characters",
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
	",\\S": "commas should be followed by whitespaces",
	# Matches incorrect structured bindings
	"&\\s+\\[": "structured bindings should not contain whitespace after the ampersand",
}.items()}
# Dict of patterns for selecting potential formatting issues in a single word.
# Also contains the error description for the patterns.
word_include = {re.compile(regex): description for regex, description in {
	# Matches any single '+', '/', '%', '=' operator that has no trailing whitespace.
	"^([^+/%=]?(?<!operator))[+/%=][^+/%=,\\s\\)\\]}]": "missing whitespace after operator",
	# Matches any series of operators ending with '=', '<' or '>' that have no trailing whitespace.
	"^[^<>=:]?[" + std_op + "]*[=<>:][^=<>:,\\s\\)\\]}]": "missing whitespace after operator",
	# Matches any '(void)' arguments in methods
	"\\(void\\)": "do not use void to denote a function with no arguments"
}.items()}

# Patterns for excluding matches (test()#match) of 'include'
match_exclude = [re.compile(regex) for regex in [
	# Matches any repeating +, - or : operators, or any ::* or ::& references
	"^.?([+:-])\\1+|::&|::\\*.?$",
	# Matches any matches which have a -> operator surrounded by at most 1 character on either side.
	"^.?->.?$",
	# Matches any matches which have a character followed by '*(' or '&('
	"^\\w[*&]\\($",
	# Matches any exponent-related matches.
	"^e[+-]\\d+$"
]]
# Patterns for excluding segments that had matches in $include
segment_exclude = [re.compile(regex) for regex in [
	# Matches anything inside '<>'; this is a bit of a hack for getting rid of type-related issues
	"<.*>",
	# Matches any visibility modes; these are followed by ':' marks.
	"^(public|protected|private|default):$"
]]
# Precompiled  helper regexes
after_comment = re.compile("[^\\s#]")
whitespace_only = re.compile("^\\s*$")
whitespaces = re.compile("\\s+")
singleLineComment = re.compile("^//(/<?)?")

# List of "" and <> includes to be treated as the other type;
# that is, any listed "" include should be grouped with <> includes,
# and vice versa.
reversed_includes = ["\"opengl.h\""]
# The list of files for which the include checks are skipped.
exclude_include_check = ["source/main.cpp"]


# A class representing error messages.
# These are stored in the error_list list to be displayed after all checks are done.
# text: the text where the error originates from
# line: the current line number
# reason: the reason for the error
class Error(object):

	def __init__(self, text, line, reason):
		self.text = text.replace('\n', '').replace('\r', '')
		self.line = line
		self.reason = reason

	def __str__(self):
		return f"\tERROR: line {self.line}: {self.reason} in '{self.text}'"

	def __lt__(self, other):
		return self.line < other.line

	def __eq__(self, other):
		return self.line == other.line and self.text == other.text and self.reason == other.reason

	def __hash__(self):
		return self.line


# A class representing warning messages.
# These are stored in the error_list list to be displayed after all checks are done.
# text: the text where the warning originates from
# line: the current line number
# reason: the reason for the warning
class Warning(Error):

	def __init__(self, text, line, reason):
		Error.__init__(self, text, line, reason)

	def __str__(self):
		return f"\tWARNING: line {self.line}: {self.reason} in '{self.text}'"


# Checks the format of all source files.
# Parameters:
# file: The path to the file being checked
# lines: The contents of the file, with the trailing line separators
# Returns: A tuple containing the list of errors and warnings found
def check_code_style(file, lines):
	issues = check_line_separators(lines)

	lines = [line.removesuffix('\n').removesuffix('\r') for line in lines]
	join(issues, check_pre_sanitize(lines, file))

	segmented_lines = join(issues, sanitize(lines))[2]
	sanitized_lines = ["".join(segments) for segments in segmented_lines]

	join(issues, check_global_format(sanitized_lines, lines, file))
	join(issues, check_local_format(sanitized_lines, segmented_lines))

	return issues


# Appends the lists in the second tuple to the lists in the first tuple. Parameters:
# first: the tuple where the lists are expanded
# second: the tuple where the lists are not expanded
# Returns the second tuple for reuse.
def join(first, second):
	for (list1, list2) in zip(first, second):
		list1 += list2
	return second


# Sanitizes the contents of the file by removing the contents of strings and comments.
# Also performs some minimal format checking that cannot be done elsewhere.
# Parameters:
# lines: the original contents of the file, without trailing line separators
# file: the path to the file
# skip_checks: whether to skip checks for formatting errors
# Returns a tuple containing the errors, warnings and the sanitized line segments.
def sanitize(lines, skip_checks=False):
	errors = []
	warnings = []

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
		# Checking for preprocessor text, except includes
		if not is_string and not is_multiline_comment and not is_char and line.lstrip().startswith("#") and not line.lstrip().startswith("#include"):
			line_segments.append(segments)
			continue
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
							errors.append(Error(line[i - 1:i + 2], line_count,
												"missing space before end of multiline comment"))
					# End of comment
					is_multiline_comment = False
					i += 1
					start_index = i + 1
				continue
			commentMatch = re.search(singleLineComment, line[i:i + 4])
			if (not is_string and commentMatch):
				segments.append(line[start_index:i].rstrip())
				if not skip_checks:
					cLen = commentMatch.end()
					# Checking for space after comment
					if len(line) > (i + cLen):
						if re.search(after_comment, line[i + cLen:i + cLen + 1]):
							errors.append(Error(line[i:i + cLen + 1], line_count,
												"missing space after beginning of single-line comment"))
				break
			elif (not is_string) and first_two == "/*":
				segments.append(line[start_index:i].rstrip())
				is_multiline_comment = True
				if not skip_checks:
					if header_found and not (
							line[i + 1:].count("*/") >= 1 and (line.endswith(")") or line.endswith("{"))):
						errors.append(Error(line.lstrip(), line_count,
											"multiline comments should only be used for the copyright header"))
					# Checking for space after comment
					if len(line) > i + 2 and line[i + 2] != ' ':
						errors.append(Error(line[i:i + 3], line_count,
											"missing space after beginning of multiline comment"))
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
	return errors, warnings, line_segments


# Tests whether the specified lines use unix-style line endings. Parameters:
# lines: the lines to test, with the terminating line separators.
# Returns a tuple of errors and warnings.
def check_line_separators(lines):
	errors = []
	warnings = []
	for index, line in enumerate(lines):
		if line.endswith("\r\n"):
			errors.append(Error(line, index + 1, "Line separators should use LF only; found CRLF"))
		elif line.endswith("\r"):
			errors.append(Error(line, index + 1, "Line separators should use LF only; found CR"))
		elif not line.endswith("\n"):
			errors.append(Error(line, index + 1, "Missing line separator"))
	return errors, warnings


# Runs checks on the contents of the file before sanitization. Parameters:
# lines: the lines of the file, without the terminating line separators. Contains the contents of strings and comments.
# file: the path to the file
# Returns a tuple of errors and warnings.
def check_pre_sanitize(lines, file):
	issues = check_line_format(lines)
	join(issues, check_copyright(lines, file))
	return issues


# Tests whether the specified file contains any formatting issues. Parameters:
# lines: the lines of the file, without terminating line separators and the contents of strings or comments
# segmented_lines: the segments of each line
# file: the path to the file
# Returns a tuple of errors and warnings.
def check_local_format(lines, segmented_lines):
	issues = ([], [])
	line_count = 0
	for line, segments in zip(lines, segmented_lines):
		line_count += 1
		line = line.lstrip()
		# Removing indentation
		if len(segments) > 0:
			segments[0] = segments[0].lstrip()
		join(issues, check_regex_format(line, segments, line_count))
	return issues


# Tests whether the specified line contains any formatting issues, based on the regex tests. Parameters:
# line: the line to test, without the contents of strings or comments
# segments: the segments of the line
# line_count: the position of the line
# Returns a tuple of errors and warnings.
def check_regex_format(line, segments, line_count):
	errors = []
	warnings = []
	# Check full-line regexes
	for regex, description in line_include.items():
		if check_match(regex, line, line):
			errors.append(Error(line, line_count, description))
	for segment in segments:
		# Skip empty
		if re.match(whitespace_only, segment):
			continue
		# Check segment regexes
		for regex, description in segment_include.items():
			if check_match(regex, segment, segment):
				errors.append(Error(segment, line_count, description))
		# Check word regexes
		for word in re.split(whitespaces, segment):
			word = word.strip()
			if word != "":
				for regex, description in word_include.items():
					if check_match(regex, word, segment):
						errors.append(Error(word, line_count, description))
	return errors, warnings


# Checks if the specified regex matches with the text. Parameters:
# regex: the regex to match
# text: the text to match
# segment: the segment the part belongs to
# Returns True if the regex matches; False otherwise.
def check_match(regex, text, segment):
	pos = re.search(regex, text)
	if pos is not None:
		match = text[pos.start():pos.end()]
		for temp in match_exclude:
			if re.search(temp, match):
				return False
		else:
			for temp in segment_exclude:
				if re.search(temp, segment):
					return False
		return True
	return False


# Checks certain global formatting properties of files, such as their copyright headers. Parameters:
# sanitized_lines: the sanitized contents of the file
# original_lines: the contents of the file, without sanitization
# file: the path to the file
# Returns a tuple of errors and warnings.
def check_global_format(sanitized_lines, original_lines, file):
	issues = ([], [])
	if file not in exclude_include_check:
		join(issues, check_include(sanitized_lines, original_lines, file))
	join(issues, check_class_forward_declarations(sanitized_lines, original_lines, file))
	return issues


# Checks if the copyright header of the file is correct. Parameters:
# lines: the lines to check, without the terminating line separators
# file: the path to the file
# Returns a tuple of errors and warnings.
def check_copyright(lines, file):
	errors = []
	warnings = []

	name = file.split("/")[-1]
	# The two halves of the copyright notice. There might be a couple lines of text separating the two halves.
	# The bool value stores whether the text is interpreted as a regex.
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
		errors.append(Error(lines[error_line], error_line + 1, "invalid or missing copyright header"))
	elif not complete:
		errors.append(Error(lines[not_found_error_line], not_found_error_line + 1,
							"invalid or incomplete copyright header"))
	return errors, warnings


# Checks whether the specified lines don't exceed the character limit, and consist of ASCII characters only. Parameters:
# lines: the lines to check, without the terminating line separators
# Returns a tuple of errors and warnings.
def check_line_format(lines):
	errors = []
	warnings = []

	line_count = 0
	for line in lines:
		line_count += 1
		if len(line) > 120:
			errors.append(Error(line, line_count, "lines should hard wrap at 120 characters"))
		for char in line:
			if ord(char) < 0 or ord(char) > 127:
				errors.append(Error(line, line_count, "files should be plain ASCII"))
				break
	return errors, warnings


# Checks the import statements at the beginning of the file. Parameters:
# sanitized_lines: the lines of the file, without the line separators and the contents of strings and comments
# original_lines: the lines of the file, without the terminating line separators
# file: the path to the file
# Returns a tuple of errors and warnings.
def check_include(sanitized_lines, original_lines, file):
	errors = []
	warnings = []

	# Replacing include statements
	for include in reversed_includes:
		stripped = include[1:-1]
		replacement = '<' + stripped + '>' if include[0] == '"' else '"' + stripped + '"'

		original_lines = [line if line != "#include " + include else "#include " + replacement for line in original_lines]

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
			warnings.append(Warning("", 0, "missing include statement for own header file"))
			return errors, warnings
		elif original_lines[groups[0][0]] != "#include \"" + name + "\"":
			warnings.append(Warning(original_lines[groups[0][0]], groups[0][0],
									"missing include for own header file"))
		if len(groups[0]) > 1:
			warnings.append(Warning(original_lines[groups[0][1]], groups[0][1],
									"missing empty line after including own header file"))
	for group in groups:
		quote = original_lines[group[0]].endswith("\"")
		for index in group:
			if original_lines[index].endswith("\"") != quote:
				warnings.append(
					Warning(original_lines[index], index, "missing empty line before changing include style"))
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
				errors.append(Error(group_lines[i], group[i] + 1, "includes are not in alphabetical order"))
	return errors, warnings


# Checks the class forward declarations within the specified file. Parameters:
# sanitized_lines: the lines of the file, without the line separators and the contents of strings and comments
# original_lines: the lines of the file, without the terminating line separators
# file: the path to the file
# Returns a tuple of errors and warnings.
def check_class_forward_declarations(sanitized_lines, original_lines, file):
	errors = []
	warnings = []

	class_lines = [(index, line) for index, line in enumerate(sanitized_lines) if line.startswith("class ") and line.endswith(';')]
	for i in range(len(class_lines) - 1):
		_, prev_line = class_lines[i]
		line_num, next_line = class_lines[i + 1]
		if prev_line.lower() > next_line.lower():
			errors.append(Error(prev_line, line_num, "class forward declarations are not in alphabetical order"))

	return errors, warnings


if __name__ == '__main__':
	errors = 0
	warnings = 0

	files = []
	if len(sys.argv[1:]) > 0:
		for pattern in sys.argv[1:]:
			files += glob.glob(pattern, recursive=True)
	else:
		files = glob.glob('source/**/*.cpp', recursive=True) + glob.glob('source/**/*.h', recursive=True) + glob.glob('tests/**/*.cpp', recursive=True) + glob.glob('tests/**/*.h', recursive=True)
	files.sort()

	for file in files:
		f = open(file, "r", newline='')
		contents = f.readlines()
		(e, w) = check_code_style(file, contents)

		errors += len(e)
		warnings += len(w)

		e = sorted(set(e))
		w = sorted(set(w))
		if e or w:
			print(file)
			if e:
				print(*e, sep='\n')
			if w:
				print(*w, sep='\n')
	print()
	text = ""
	if errors > 0:
		text += "Found " + str(errors) + " formatting " + ("error" if errors == 1 else "errors")
		if warnings > 0:
			text += " and " + str(warnings) + " " + ("warning" if warnings == 1 else "warnings")
		text += "."
		print(text)
		exit(1)
	if warnings == 0:
		print("No formatting errors found.")
	else:
		print(warnings, "warning" if warnings == 1 else "warnings", "found.")
	exit(0)

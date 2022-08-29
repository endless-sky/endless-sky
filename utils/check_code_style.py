#!/usr/bin/python

import regex as re
import glob

# Script that checks for common code formatting pitfalls not covered by clang-format or other tests.

# String version of the regexes for easy editing
# List of the standard operators that are checked
stdOp = "+/*<>&%=|!:-"
# Dict of patterns for selection potential formatting issues in full lines.
# These lines don't contain the contents of strings, chars or comments.
# The dict also contains the error description for the patterns.
line_include = {
	# Matches any '{' following an 'if', 'for' or 'switch' statement.
	"^(?:if|for|switch).*{$": "'{' should be on new line",
	# Matches any '{' not preceded by a whitespace or '(', except when the '{' is closed on the same line.
	"(?<!^(struct|inline).*)[^\\s(]+{(?!.*})": "missing whitespace before '{'"
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
	# except if the whitespace is followed by a semicolon.
	# Due to the way segments work, it allows strings and comments to begin after the space.
	"(?![A-Z]+)^.*\\(\\s(?!;)": "missing whitespace after opening parenthesis",
	# Matches any parenthesis preceded by a whitespace,
	# except if the whitespace follows a semicolon.
	"[^;]\\s\\)]": "missing whitespace before closing parenthesis",
	# Matches any whitespaces at the end of a line that are preceded by brackets, parentheses or semicolons.
	"[;[{]\\s+$": "trailing whitespace at end of line",
	# Matches any 'if', 'for' or 'switch' statements where the '(' is preceded by a whitespace.
	"(?:if|switch|for)\\s+\\(": "extra whitespace before '('"
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
	"^[^<>=:]?[" + stdOp + "]+[=<>:][^=<>:,\\s]": "missing whitespace after operator",
	# Matches a single ':' operator that is followed by a ')' in the same word,
	# as seen in this incorrectly formatted example: for(int a :b)
	# This cannot be merged with the previous one due to the use of ':' in method implementations.
	"^:[\\S]+\\)$": "missing whitespace after operator"
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

errors = 0


def check_code_style():
	files = glob.glob('**/*.cpp', recursive=True) + glob.glob('**/*.h', recursive=True)
	for file in files:
		test_file(file)


# Tests a file for syntax errors. Parameters:
# file: The path of the file


def test_file(file):
	is_multiline_comment = False
	is_string = False
	is_char = False
	is_raw_string = False
	line_count = 0
	f = open(file, "r")
	lines = f.readlines()
	for line in lines:
		segments = []
		line = line.lstrip().replace('\n', '').replace('\r', "")
		line_count += 1
		is_escaped = False
		# Start index is the beginning of the sequence to be tested
		start_index = 0
		# Looking for parts of the file that are not strings or comments
		for i in range(len(line)):
			# Getting current character
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
				else:
					if is_string:
						start_index = i
					else:
						segments.append(line[start_index:i + 1])
					is_string = not is_string
		else:
			if (not is_multiline_comment) and (not is_char) and (not is_escaped) and (not is_string):
				segments.append(line[start_index:])
			test(segments, file, line_count)


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
		global errors
		errors += 1
		return True
	return False


def write_error(text, file, line, reason):
	print("Formatting error in file", file, "line", line.__str__() + ":", text)
	print("\tReason:", reason)


if __name__ == '__main__':
	check_code_style()
	if errors > 0:
		print("Found", errors, "formatting", "error" if errors == 1 else "errors")
		exit(1)
	print("No formatting errors found.")
	exit(0)

#!/usr/bin/env python3
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

import argparse
import glob
import sys
from pathlib import Path
from typing import List, Tuple, Dict, Set, Pattern, Optional

# Use the 'regex' module which has better Unicode support and more features (like improved lookbehind)
# than the standard 're' module. This script relies on it.
try:
	import regex
except ImportError:
	print("Error: The 'regex' module is required. Please install it ('pip install regex')", file=sys.stderr)
	sys.exit(1)

# Script that checks for common C++ code formatting pitfalls not covered by clang-format or other tests.
# The formatting rules are generally based on the guide found at http://endless-sky.github.io/styleguide/styleguide.xml
# Unit tests mandate the existence of several exceptions to these rules.
#
# This checker uses regular expressions. For the sake of simplicity, these expressions represent a rather loose
# interpretation of the rules and may have false positives/negatives in complex C++ constructs.

# --- Configuration ---

# Standard operators checked for spacing
STD_OPERATORS: str = r"\+/\\*<>&%=\\|!:\-"

# --- Regex Definitions ---
# Pre-compile regex patterns for efficiency.

# Patterns for checking potential formatting issues in *full lines* (after removing comments/strings).
# Key: Compiled regex pattern
# Value: Error description string
LINE_INCLUDE_PATTERNS: Dict[Pattern[str], str] = {
	# Matches '{' immediately following control flow keywords (if, else, for, while, switch, catch)
	# e.g., if (condition) {
	regex.compile(r"^(else\sif|if|else|for|switch|catch|while)\s?\(.*\)\s*\{$"):
		"'{' should be on a new line after control structure",

	# Matches '{' not preceded by whitespace or '(', unless it's closed on the same line ' T{...}; '
	# or part of struct/inline definitions where it's allowed.
	# e.g., void func(){ vs void func() {
	regex.compile(r"(?<!^(?:struct|inline|class)\s.*)[^\s(]\{(?!.*\})"):
		"missing whitespace before '{'",

	# Matches closing parenthesis ')' preceded by whitespace, unless it follows a semicolon (like in for loops)
	# or an all-caps "macro-like" function call.
	# e.g., func(arg ) vs func(arg)
	regex.compile(r"(?<![A-Z])(?<!;)\s+\)$"):
		"extra whitespace before closing parenthesis ')'",

	# Matches control flow keywords not at the beginning of a line (ignoring indentation),
	# except for 'else if' naturally following 'else'. Also ignores inline functions.
	# e.g., x = 1; if (y) ... vs x = 1;\nif (y) ...
	regex.compile(r"(?<!^inline\s.*)(?<!^\s*)[^\w\s]\s*((?<!else\s)if|else\sif|else|switch|for|catch|try|do)(\s*\{|\s*\()"):
		"statement should begin on a new line",

	# Matches semicolons not at the end of the line (ignoring trailing whitespace),
	# unless part of a for loop condition `for(...;...;...)`.
	# e.g., x = 1; y = 2; vs x = 1;\ny = 2;
	regex.compile(r";(?!\s*$)(?![^()]*\))"):  # Adjusted to better handle for loops
		"semicolon should generally terminate the line (outside for-loops)",

	# Matches trailing whitespace at the end of a line.
	regex.compile(r"\s+$"):
		"trailing whitespace at end of line",

	# Matches operators without preceding whitespace, with many exceptions:
	# - Not at the start of a line (unary operators like *ptr, ++i).
	# - Not after '(', '[', '{'.
	# - Not part of 'case' constant expressions (like case x+1:).
	# - Not part of 'operator' overloads (like operator++).
	# - Not '->', '::', '.*' (member access).
	# - Not followed by another operator immediately (like x = +y; or a+=b;)
	# - Not part of C++20 three-way comparison `<=>` (though `std_op` doesn't include it fully)
	# - Not part of ellipsis `...`
	# e.g., x=y vs x = y
	# This regex is complex and prone to false positives/negatives.
	regex.compile(
		r"(?<!^case\s.*)"  # Ignore case statements
		r"(?<!^.*[^a-zA-Z0-9_]?operator)"  # Ignore operator definitions
		r"(?<![([{\s" + STD_OPERATORS + r"])"  # Must not be preceded by space, bracket, or another operator
		r"[" + STD_OPERATORS + r"]+"  # Match one or more standard operators
		r"(?<!->|::|\.\*)" # Exclude member access operators
		r"(?<!\+\+|--)" # Exclude increment/decrement attached to operand
		r"(?![=\s" + STD_OPERATORS + r"])"  # Must not be followed by = or another operator immediately
		r"(?!.*\))" # Avoid matching inside simple function calls like func(a*b) - imperfect
		r"(?!\.\.\.)" # Ignore ellipsis
	):
		"missing whitespace before operator"
}

# Patterns for checking potential formatting issues within a code *segment*
# (part of a line between comments/strings).
SEGMENT_INCLUDE_PATTERNS: Dict[Pattern[str], str] = {
	# Matches 2+ consecutive whitespace characters, unless the line primarily consists of
	# comma-separated values (like array initializers) allowing manual alignment.
	regex.compile(r"(?!^[\s\w\.,\d{}+\-/*]*$)^.*\S\s{2,}"):
		"multiple consecutive whitespace characters (use tabs/single spaces)",

	# Matches opening parenthesis '(' followed by whitespace, unless it's immediately followed by a semicolon ';'
	# or part of an ALL_CAPS macro call.
	# e.g., func( arg ) vs func(arg)
	regex.compile(r"(?<![A-Z])\(\s+(?!;)"):
		"extra whitespace after opening parenthesis '('",

	# Matches control flow keywords followed by whitespace before the opening '('.
	# e.g., if ( vs if( (Note: clang-format usually handles this)
	regex.compile(r"\b(if|switch|for|while|catch)\s+\("):
		"extra whitespace between keyword and opening parenthesis '('",

	# Matches 'try' or 'do' at the end of a line without a '{' on the same line.
	# e.g., try\n{ vs try {
	regex.compile(r"^(?:try|do)$"):
		"'try'/'do' and opening '{' should typically be on the same line",

	# Matches literal tab characters within a code segment (tabs should only be for indentation).
	regex.compile(r"\t"):
		"tab characters should only be used for indentation, not alignment",

	# Matches commas not followed by whitespace or the end of the line.
	# e.g., func(a,b) vs func(a, b)
	regex.compile(r",(?!\s|$)"):
		"comma ',' should be followed by whitespace or end of line"
}

# Patterns for checking potential formatting issues within a single *word* or operator group.
WORD_INCLUDE_PATTERNS: Dict[Pattern[str], str] = {
	# Matches single '+', '/', '%', '=' operators not followed by whitespace (or specific chars).
	# Ignores cases like operator+=.
	# e.g., x = y+z vs x = y + z
	regex.compile(r"^(?<!operator)[+\-*/%](?![=\s\)\],}>])"): # Slightly refined
		"missing whitespace after binary operator (+, -, *, /, %)",

	# Matches comparison/assignment operators (including multi-char like <=, +=) not followed by whitespace.
	# e.g., x+=y vs x += y, a<b vs a < b
	regex.compile(r"^(?:[^<>=:!+-]|\b)\s*[" + STD_OPERATORS + r"]*?[=<>&|:](?![=<>\s\)\],}a-zA-Z0-9])"): # Refined
		"missing whitespace after comparison or assignment operator",

	# Matches '(void)' parameter lists.
	# e.g., func(void) vs func()
	regex.compile(r"\(\s*void\s*\)"):
		"use '()' instead of '(void)' for functions with no arguments"
}

# --- Exclusion Patterns ---
# Regexes to exclude specific matches that were flagged by the INCLUDE patterns above but are actually valid.

# Exclude matches based on the specific text matched by an INCLUDE pattern.
MATCH_EXCLUDE_PATTERNS: List[Pattern[str]] = [
	# Ignore repeated operators like ++, --, ::, <<, >>
	regex.compile(r"^([+:-])\1+$"),
	# Ignore ::* or ::& (pointer/reference to member)
	regex.compile(r"::[\*&]"),
	# Ignore ->, ->* operators
	regex.compile(r"->\*?"),
	# Ignore * or & when part of type declarations or dereference/address-of, especially near parentheses.
	# e.g. int* p; func(&x); (void(*)())
	regex.compile(r"^\w*[*&]\(?$"), # pointer/ref declaration or address-of
	regex.compile(r"^\)[*&]"), # function pointer return type
	# Ignore scientific notation like 1e+10, 2E-5
	regex.compile(r"[eE][+\-]\d+$")
]

# Exclude entire segments if they match these patterns (often template/type related).
SEGMENT_EXCLUDE_PATTERNS: List[Pattern[str]] = [
	# Ignore anything inside <> (templates, includes)
	regex.compile(r"<.*>"),
	# Ignore C++ access specifiers followed by ':'
	regex.compile(r"^(public|protected|private):$"),
	# Ignore labels followed by ':'
	regex.compile(r"^\w+:$")
]

# --- Helper Regexes ---
AFTER_COMMENT_START: Pattern[str] = regex.compile(r"[^\s#/]") # Checks for non-whitespace after // or /*
WHITESPACE_ONLY: Pattern[str] = regex.compile(r"^\s*$")
WHITESPACES: Pattern[str] = regex.compile(r"\s+")
SINGLE_LINE_COMMENT: Pattern[str] = regex.compile(r"^//(/<?)?") # Matches //, ///, //<

# --- Configuration for Include Checks ---
# List of "" includes to be treated as <> includes, and vice versa during sorting/grouping checks.
REVERSED_INCLUDES: Set[str] = {
	"\"opengl.h\""
}
# Files to skip include order/style checks for.
EXCLUDE_INCLUDE_CHECK: Set[str] = {
	"source/main.cpp" # Example from original code
}

# --- Data Structures ---

class Issue:
	"""Base class for representing code style issues."""
	def __init__(self, text: str, line_num: int, reason: str):
		# Strip newlines and leading/trailing whitespace for cleaner output
		self.text: str = text.strip().replace('\n', '').replace('\r', '')
		self.line_num: int = line_num
		self.reason: str = reason

	def __str__(self) -> str:
		# Overridden by subclasses
		return f"line {self.line_num}: {self.reason} in '{self.text}'"

	def __lt__(self, other: 'Issue') -> bool:
		"""Sort issues primarily by line number."""
		if not isinstance(other, Issue):
			return NotImplemented
		return self.line_num < other.line_num

	def __eq__(self, other: object) -> bool:
		"""Check for equality based on line, reason, and text content."""
		if not isinstance(other, Issue):
			return NotImplemented
		return (self.line_num == other.line_num and
				self.reason == other.reason and
				self.text == other.text)

	def __hash__(self) -> int:
		"""Generate a hash based on line, reason, and text content."""
		return hash((self.line_num, self.reason, self.text))

class Error(Issue):
	"""Represents a style error."""
	def __str__(self) -> str:
		return f"\tERROR: line {self.line_num}: {self.reason} in '{self.text}'"

class Warning(Issue):
	"""Represents a style warning."""
	def __str__(self) -> str:
		return f"\tWARNING: line {self.line_num}: {self.reason} in '{self.text}'"

# --- Core Logic Functions ---

def check_code_style(file_path: Path, lines: List[str]) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks the format of a single source file.

	Args:
		file_path: The path to the file being checked.
		lines: The content of the file as a list of strings, including line separators.

	Returns:
		A tuple containing a list of Errors and a list of Warnings found.
	"""
	all_errors: List[Error] = []
	all_warnings: List[Warning] = []

	def combine_issues(new_issues: Tuple[List[Error], List[Warning]]):
		"""Helper to append new issues to the main lists."""
		all_errors.extend(new_issues[0])
		all_warnings.extend(new_issues[1])

	# 1. Check line separators first (uses original lines with separators)
	combine_issues(check_line_separators(lines))

	# 2. Prepare lines without separators for further checks
	lines_no_sep: List[str] = [line.rstrip('\r\n') for line in lines]

	# 3. Run checks that need the original line content (including comments/strings)
	combine_issues(check_pre_sanitize(lines_no_sep, file_path))

	# 4. Sanitize lines (remove comments/strings) and get segmented lines
	sanitize_errors, sanitize_warnings, segmented_lines = sanitize(lines_no_sep)
	all_errors.extend(sanitize_errors)
	all_warnings.extend(sanitize_warnings)
	sanitized_lines: List[str] = [" ".join(segments) for segments in segmented_lines] # Join segments for line-based checks

	# 5. Run checks on sanitized code
	combine_issues(check_global_format(sanitized_lines, lines_no_sep, file_path))
	combine_issues(check_local_format(sanitized_lines, segmented_lines)) # Pass both versions

	# Remove duplicates and sort
	unique_errors = sorted(list(set(all_errors)))
	unique_warnings = sorted(list(set(all_warnings)))

	return unique_errors, unique_warnings


def sanitize(lines: List[str], skip_checks: bool = False) -> Tuple[List[Error], List[Warning], List[List[str]]]:
	"""
	Removes comments and string literals from lines, returning segmented lines.
	Also performs some checks during the process.

	Args:
		lines: File content as a list of strings (no line separators).
		skip_checks: If True, avoids performing checks during sanitization.

	Returns:
		A tuple containing:
		- List of Errors found during sanitization.
		- List of Warnings found during sanitization.
		- A list of lists of strings, where each inner list contains the code
		  segments of the corresponding original line.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []
	line_segments: List[List[str]] = []

	in_multiline_comment: bool = False
	in_string: bool = False
	in_char: bool = False
	in_raw_string_delim: Optional[str] = None # Stores the delimiter for R"delim(...)delim"
	is_escaped: bool = False
	copyright_header_found: bool = False

	for line_num, line in enumerate(lines, 1):
		segments: List[str] = []
		current_segment_start: int = 0
		i: int = 0
		line_len = len(line)

		while i < line_len:
			char = line[i]

			# --- Handle Escaped Characters ---
			if is_escaped:
				is_escaped = False
				i += 1
				continue
			if char == '\\':
				is_escaped = True
				i += 1
				continue

			# --- Handle Multi-line Comments ---
			if in_multiline_comment:
				if char == '*' and i + 1 < line_len and line[i+1] == '/':
					in_multiline_comment = False
					if not skip_checks:
						# Check for space before '*/' (unless at start of line segment)
						prev_char_idx = i - 1
						if prev_char_idx >= current_segment_start and line[prev_char_idx] not in (' ', '\t', '*'):
							errors.append(Error(line[prev_char_idx:i+2], line_num,
												"missing space before end of multiline comment '*/'"))
					current_segment_start = i + 2
					i += 2
				else:
					i += 1
				continue # Skip rest of checks inside multiline comment

			# --- Handle Single-line Comments ---
			if not in_string and not in_char and char == '/' and i + 1 < line_len:
				if line[i+1] == '/':
					# Found '//'
					comment_match = SINGLE_LINE_COMMENT.match(line[i:])
					if comment_match:
						# Add segment before comment start
						segments.append(line[current_segment_start:i].rstrip())
						if not skip_checks:
							# Check for space after '//' marker
							comment_marker_end = i + comment_match.end()
							if comment_marker_end < line_len and AFTER_COMMENT_START.match(line[comment_marker_end:comment_marker_end+1]):
								errors.append(Error(line[i:comment_marker_end+1], line_num,
													"missing space after beginning of single-line comment marker"))
						current_segment_start = line_len # Skip rest of the line
						break # End processing for this line
				elif line[i+1] == '*':
					# Found '/*'
					segments.append(line[current_segment_start:i].rstrip())
					in_multiline_comment = True
					if not skip_checks:
						# Allow multiline comments only for the copyright header or specific block comments
						# Heuristic: Check if it's likely a copyright header or a function/block explanation
						is_likely_header = not copyright_header_found and line_num < 20
						is_likely_block_comment = line.lstrip().startswith("/*") and line.rstrip().endswith("*/")

						# Check for space after '/*'
						if i + 2 < line_len and line[i+2] not in (' ', '\t', '*'):
							errors.append(Error(line[i:i+3], line_num,
												"missing space after beginning of multiline comment '/*'"))

						if not is_likely_header and not is_likely_block_comment:
							# Check if it closes on the same line and follows code - might be problematic
							if "*/" in line[i+2:] and line[i-1:i].strip():
								 errors.append(Error(line.strip(), line_num,
													"avoid inline '/* ... */' comments after code; use '//' instead"))
							elif not is_likely_block_comment: # If not clearly a header or block comment, warn
								# Warning instead of error, as block comments are sometimes used.
								warnings.append(Warning(line.strip(), line_num,
												   "multiline comments '/* ... */' are primarily for copyright headers; prefer '//'"))

					copyright_header_found = True # Assume any top-level multiline comment could be the header start
					current_segment_start = i + 2
					i += 2
					continue # Restart checks for the character after '/*'

			# --- Handle Strings and Chars ---
			if not in_char and char == '"':
				# Check for raw string literals: R"delim(...)delim"
				if line.startswith('R"', i-1) and i > 0 and line[i-1] in ('u', 'U', 'L', 'u8'): # Handle prefixes uR", LR", etc.
					prefix_len = 1
				elif line.startswith('R"', i):
					prefix_len = 0
				else:
					prefix_len = -1 # Not a raw string start

				if prefix_len != -1:
					raw_start_index = i + prefix_len + 1 # After the R"
					delim_match = regex.match(r'^([a-zA-Z0-9_]*)(\()', line[raw_start_index:])
					if delim_match:
						in_raw_string_delim = delim_match.group(1)
						closing_delim = ")" + in_raw_string_delim + '"'
						# Find the end of the raw string
						raw_end_index = line.find(closing_delim, raw_start_index + delim_match.end())
						if raw_end_index != -1:
							# Found the end, skip over the raw string content
							# Add segment before raw string
							segments.append(line[current_segment_start : i - prefix_len])
							# Move index past the raw string
							i = raw_end_index + len(closing_delim)
							current_segment_start = i
							in_raw_string_delim = None # Reset raw string state
							continue # Continue loop from the character after the raw string
						else:
							# Raw string doesn't close on this line, treat rest of line as string content
							segments.append(line[current_segment_start : i - prefix_len])
							in_string = True # Mark as inside a string to consume rest of line
							# We don't perfectly track multiline raw strings across lines, but consume this line
							current_segment_start = line_len
							break # End processing for this line
					else:
						# Malformed R" literal? Treat as normal string start
						pass # Fall through to normal string handling

				# Normal string handling
				if in_string:
					# End of string
					in_string = False
					current_segment_start = i + 1
				else:
					# Start of string
					segments.append(line[current_segment_start:i]) # Add segment before the opening quote
					in_string = True
				i += 1
				continue

			elif not in_string and char == "'":
				if in_char:
					# End of char literal
					in_char = False
					current_segment_start = i + 1
				else:
					# Start of char literal
					segments.append(line[current_segment_start:i]) # Add segment before opening quote
					in_char = True
				i += 1
				continue

			# If inside a string or char literal, just advance
			if in_string or in_char:
				i += 1
				continue

			# --- Default: Just advance character ---
			i += 1

		# --- End of Line Processing ---
		if current_segment_start < line_len and not in_multiline_comment and not in_string and not in_char:
			# Add any remaining part of the line as a segment
			segments.append(line[current_segment_start:].rstrip())

		# Filter out empty segments potentially added by rstrip()
		line_segments.append([seg for seg in segments if seg])

	# Check if a multiline comment was left open at EOF
	if in_multiline_comment:
		errors.append(Error("", len(lines), "end of file reached inside a multiline comment"))
	if in_string:
		errors.append(Error("", len(lines), "end of file reached inside a string literal"))
	if in_char:
		errors.append(Error("", len(lines), "end of file reached inside a char literal"))


	return errors, warnings, line_segments


def check_line_separators(lines: List[str]) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks if lines use Unix-style (LF) line endings.

	Args:
		lines: The list of lines read from the file, preserving original endings.

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []
	for i, line in enumerate(lines):
		line_num = i + 1
		if line.endswith("\r\n"):
			errors.append(Error(line.strip(), line_num, "Line separators should use LF only; found CRLF"))
		elif line.endswith("\r"):
			errors.append(Error(line.strip(), line_num, "Line separators should use LF only; found CR"))
		# We don't check for missing final newline here, git often handles this.
		# elif not line.endswith("\n") and i < len(lines) - 1: # Check all lines except potentially the last
		#     errors.append(Error(line.strip(), line_num, "Missing LF line separator"))
	return errors, warnings


def check_pre_sanitize(lines: List[str], file_path: Path) -> Tuple[List[Error], List[Warning]]:
	"""
	Runs checks on the original line content *before* comments/strings are removed.

	Args:
		lines: File content as a list of strings (no line separators).
		file_path: Path object for the file being checked.

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []

	line_format_errors, line_format_warnings = check_line_format(lines)
	errors.extend(line_format_errors)
	warnings.extend(line_format_warnings)

	copyright_errors, copyright_warnings = check_copyright(lines, file_path)
	errors.extend(copyright_errors)
	warnings.extend(copyright_warnings)

	return errors, warnings


def check_local_format(
	sanitized_lines: List[str],
	segmented_lines: List[List[str]]
) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks formatting rules on sanitized lines and segments.

	Args:
		sanitized_lines: List of lines with comments/strings removed and segments joined.
		segmented_lines: List of lists of code segments for each line.

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []

	for line_num, (full_line, segments) in enumerate(zip(sanitized_lines, segmented_lines), 1):
		# --- Check Full Line Patterns ---
		# We use the joined sanitized line here, stripped of leading whitespace for checks.
		line_stripped = full_line.lstrip()
		if not line_stripped: # Skip empty or whitespace-only lines
			continue

		for pattern, description in LINE_INCLUDE_PATTERNS.items():
			match = pattern.search(line_stripped)
			if match and not is_match_excluded(match.group(0), line_stripped):
				errors.append(Error(line_stripped, line_num, description))

		# --- Check Segment and Word Patterns ---
		for segment in segments:
			segment_stripped = segment.strip()
			if not segment_stripped: # Skip empty segments
				continue

			# Exclude segments matching exclusion patterns (like templates)
			if any(pattern.search(segment_stripped) for pattern in SEGMENT_EXCLUDE_PATTERNS):
				continue

			# Check segment-level patterns
			for pattern, description in SEGMENT_INCLUDE_PATTERNS.items():
				match = pattern.search(segment) # Use original segment for context checks if needed
				if match and not is_match_excluded(match.group(0), segment):
						errors.append(Error(segment_stripped, line_num, description))

			# Check word-level patterns
			# Split segment by whitespace, process each 'word'
			words = WHITESPACES.split(segment_stripped)
			for word in words:
				if not word: continue # Skip empty strings resulting from split
				for pattern, description in WORD_INCLUDE_PATTERNS.items():
					match = pattern.search(word)
					# Pass the 'word' as the match context, and the 'segment' as the wider context
					if match and not is_match_excluded(match.group(0), segment_stripped):
						errors.append(Error(word, line_num, description))

	return errors, warnings


def is_match_excluded(matched_text: str, context_segment: str) -> bool:
	"""
	Checks if a matched text or its context segment should be excluded based on exclusion patterns.

	Args:
		matched_text: The specific text that matched an INCLUDE pattern.
		context_segment: The full segment where the match occurred.

	Returns:
		True if the match should be excluded, False otherwise.
	"""
	# Check if the specific matched text is excluded
	if any(pattern.search(matched_text) for pattern in MATCH_EXCLUDE_PATTERNS):
		return True
	# Check if the entire segment the match belongs to is excluded
	if any(pattern.search(context_segment) for pattern in SEGMENT_EXCLUDE_PATTERNS):
		return True
	return False


def check_global_format(
	sanitized_lines: List[str],
	original_lines: List[str],
	file_path: Path
) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks formatting properties that require a view of the whole file, like include order.

	Args:
		sanitized_lines: Sanitized lines (comments/strings removed).
		original_lines: Original lines (no line separators).
		file_path: Path object for the file.

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []

	if file_path.name not in EXCLUDE_INCLUDE_CHECK:
		include_errors, include_warnings = check_include_order(sanitized_lines, original_lines, file_path)
		errors.extend(include_errors)
		warnings.extend(include_warnings)

	# Add other global checks here if needed in the future

	return errors, warnings


def check_copyright(lines: List[str], file_path: Path) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks if the file starts with the expected copyright header.

	Args:
		lines: File content as list of strings (no line separators).
		file_path: Path object for the file.

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []
	file_name = file_path.name

	# Define the expected structure of the copyright header.
	# Use None in the list to indicate a line that can vary but should be checked by regex.
	# Use "" for expected empty lines.
	# The regex checks for the copyright year(s) and author format.
	expected_header_prefix = [
		f"/* {file_name}",
		# Regex for "Copyright (c) YYYY" or "Copyright (c) YYYY-YYYY" or "Copyright (c) YYYY, YYYY" by Author Name"
		regex.compile(r"^\s*Copyright \(c\) \d{4}(?:(?:-|, )\d{4})? by .*")
	]
	expected_header_suffix = [
		"",
		"Endless Sky is free software: you can redistribute it and/or modify it under the",
		"terms of the GNU General Public License as published by the Free Software",
		"Foundation, either version 3 of the License, or (at your option) any later version.",
		"",
		"Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY",
		"WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A",
		"PARTICULAR PURPOSE. See the GNU General Public License for more details.",
		"",
		"You should have received a copy of the GNU General Public License along with",
		"this program. If not, see <https://www.gnu.org/licenses/>.",
		"*/",
		""
	]

	if len(lines) < len(expected_header_prefix) + len(expected_header_suffix):
		errors.append(Error(f"File: {file_name}", 1, "File is too short to contain the standard copyright header."))
		return errors, warnings

	# Check prefix
	line_idx = 0
	for expected in expected_header_prefix:
		actual_line = lines[line_idx].strip() # Strip surrounding whitespace for comparison
		if isinstance(expected, regex.Pattern):
			if not expected.match(actual_line):
				errors.append(Error(actual_line or "<empty line>", line_idx + 1, f"Copyright line mismatch. Expected pattern: {expected.pattern}"))
				return errors, warnings # Stop early if header start is wrong
		elif isinstance(expected, str):
			# Trim expected line for comparison unless it's meant to be empty
			expected_compare = expected.strip() if expected else ""
			actual_compare = actual_line.replace("/* ", "/*", 1) if expected.startswith("/* ") else actual_line # Handle potential space after /*
			if expected_compare != actual_compare:
				errors.append(Error(actual_line or "<empty line>", line_idx + 1, f"Copyright header mismatch. Expected: '{expected}'"))
				return errors, warnings # Stop early
		line_idx += 1

	# Find where the suffix starts
	suffix_start_line = -1
	# Allow a few lines gap between the copyright year line and the start of the GPL text block
	max_gap = 5
	for gap in range(max_gap + 1):
		potential_start_idx = line_idx + gap
		if potential_start_idx < len(lines) and lines[potential_start_idx].strip() == expected_header_suffix[0]:
			# Check if the *next* line matches the start of the fixed text block
			if (potential_start_idx + 1 < len(lines) and
				lines[potential_start_idx + 1].strip() == expected_header_suffix[1]):
				suffix_start_line = potential_start_idx
				break

	if suffix_start_line == -1:
		errors.append(Error(lines[line_idx] if line_idx < len(lines) else "<EOF>", line_idx + 1,
							"Could not find the start of the GPL notice block in the header."))
		return errors, warnings

	# Check suffix
	line_idx = suffix_start_line
	for expected in expected_header_suffix:
		if line_idx >= len(lines):
			errors.append(Error("<EOF>", line_idx + 1, "Copyright header is incomplete (ends prematurely)."))
			return errors, warnings

		actual_line = lines[line_idx].strip()
		expected_compare = expected.strip() if expected else ""
		if expected_compare != actual_line:
			errors.append(Error(actual_line or "<empty line>", line_idx + 1, f"Copyright header mismatch. Expected: '{expected}'"))
			return errors, warnings # Stop early
		line_idx += 1

	return errors, warnings


def check_line_format(lines: List[str]) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks for line length and non-ASCII characters.

	Args:
		lines: File content as list of strings (no line separators).

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []
	max_line_length = 120

	for line_num, line in enumerate(lines, 1):
		# Check line length (consider tabs as potentially > 1 char, but simple check first)
		# A more accurate check would replace tabs with spaces according to project style (e.g., 4 spaces)
		if len(line) > max_line_length:
			# Show only the start of the long line for brevity
			errors.append(Error(line[:max_line_length//2] + "...", line_num,
								f"Line exceeds {max_line_length} characters (length: {len(line)})"))

		# Check for non-ASCII characters
		for char_idx, char in enumerate(line):
			if ord(char) > 127:
				errors.append(Error(line, line_num, f"Non-ASCII character '{char}' (code {ord(char)}) found at column {char_idx+1}. Files should be plain ASCII."))
				# Report only the first non-ASCII char per line
				break
	return errors, warnings


def check_include_order(
	sanitized_lines: List[str],
	original_lines: List[str],
	file_path: Path
) -> Tuple[List[Error], List[Warning]]:
	"""
	Checks #include directives for style and alphabetical order within groups.

	Args:
		sanitized_lines: Sanitized lines (comments/strings removed).
		original_lines: Original lines (no line separators).
		file_path: Path object for the file.

	Returns:
		A tuple containing lists of Errors and Warnings.
	"""
	errors: List[Error] = []
	warnings: List[Warning] = []
	is_cpp_file = file_path.suffix == ".cpp"
	header_name = file_path.with_suffix(".h").name

	# Find all #include lines and their original text
	include_directives: List[Tuple[int, str]] = []
	for line_num, (sanitized, original) in enumerate(zip(sanitized_lines, original_lines), 1):
		if sanitized.startswith("#include"):
			include_directives.append((line_num, original.strip()))

	if not include_directives:
		# No includes found, maybe warn if it's a cpp file expecting its own header?
		if is_cpp_file and header_name[0].isupper(): # Heuristic: Uppercase suggests class/component header
			# Check if the corresponding .h file actually exists before warning
			# This requires filesystem access relative to the project root usually
			# For simplicity, we'll keep the original warning logic for now.
			warnings.append(Warning(f"File: {file_path.name}", 1, f"Potentially missing include for own header '{header_name}'"))
		return errors, warnings

	# Group includes separated by blank lines or other code
	include_groups: List[List[Tuple[int, str]]] = []
	if include_directives:
		current_group = [include_directives[0]]
		for i in range(1, len(include_directives)):
			prev_line_num = include_directives[i-1][0]
			current_line_num = include_directives[i][0]
			# If lines are not consecutive, start a new group
			if current_line_num != prev_line_num + 1:
				include_groups.append(current_group)
				current_group = [include_directives[i]]
			else:
				current_group.append(include_directives[i])
		include_groups.append(current_group) # Add the last group

	# --- Check Own Header Include (for .cpp files) ---
	if is_cpp_file and header_name[0].isupper():
		expected_own_include = f'#include "{header_name}"'
		first_group = include_groups[0]
		first_include_line_num, first_include_text = first_group[0]

		if first_include_text != expected_own_include:
			warnings.append(Warning(first_include_text, first_include_line_num,
									f"First include should be own header '{header_name}'. Found: {first_include_text}"))
		elif len(first_group) > 1:
			second_include_line_num, second_include_text = first_group[1]
			warnings.append(Warning(second_include_text, second_include_line_num,
									"Missing empty line after including own header file."))

		# Remove the first group if it was the own header group for subsequent checks
		if first_include_text == expected_own_include:
			include_groups = include_groups[1:]

	# --- Check Grouping and Sorting ---
	for group in include_groups:
		if not group: continue

		group_lines = [(num, text) for num, text in group]
		original_style = None # Track if group is "" or <>

		# Normalize for sorting: treat reversed includes, convert to common format for comparison
		sortable_group: List[Tuple[str, int, str]] = []
		for line_num, text in group_lines:
			include_path_match = regex.search(r'#include\s+([<"])(.*)[>"]', text)
			if not include_path_match:
				errors.append(Error(text, line_num, f"Malformed #include directive: {text}"))
				continue # Skip malformed includes

			bracket_type = include_path_match.group(1)
			include_path = include_path_match.group(2)
			full_include_spec = f"{bracket_type}{include_path}{'>' if bracket_type == '<' else '"'}"

			# Determine effective style (<> or "") considering reversals
			current_style = '"' if bracket_type == '"' else '<'
			if full_include_spec in REVERSED_INCLUDES:
				current_style = '<' if current_style == '"' else '"'

			if original_style is None:
				original_style = current_style
			elif current_style != original_style:
				warnings.append(Warning(text, line_num,
										f"Include style changed within a group (from {original_style} to {current_style}). Separate groups with a blank line."))
				# Don't break, continue checking sorting within the potentially mixed group

			# Create sort key: Use lowercase path, ignore leading slashes/dots for sorting.
			# Prefer base name sorting if paths are involved.
			sort_key_path = include_path.split('/')[-1].lower()
			sortable_group.append((sort_key_path, line_num, text))

		# Check alphabetical order within the group based on the sort key
		sorted_group = sorted(sortable_group, key=lambda item: item[0])

		for i in range(len(sortable_group)):
			original_pos_item = sortable_group[i]
			sorted_pos_item = sorted_group[i]
			# Compare the original text and line number to detect misordering
			if original_pos_item[2] != sorted_pos_item[2]:
				# Find where the current item *should* be
				correct_item = sorted_pos_item
				errors.append(Error(original_pos_item[2], original_pos_item[1],
									f"Include not in alphabetical order within its group. Should be closer to '{correct_item[2]}'."))
				# Avoid reporting cascading errors for the same out-of-place item
				break # Only report the first misordered item in a group

	return errors, warnings

# --- Main Execution ---

if __name__ == '__main__':
	parser = argparse.ArgumentParser(
		description="Check C++ code style for Endless Sky formatting rules not covered by clang-format.",
		epilog="Checks for issues like line endings, copyright headers, include order, spacing, and more."
	)
	parser.add_argument(
		'files',
		metavar='FILE_OR_PATTERN',
		nargs='*',
		help='Specific files or glob patterns (e.g., "source/**/*.cpp") to check. If omitted, checks default paths.'
	)
	parser.add_argument(
		'-v', '--verbose',
		action='store_true',
		help='Enable more verbose output (currently unused, reserved for future).'
	)

	args = parser.parse_args()

	total_errors: int = 0
	total_warnings: int = 0
	files_to_check: List[Path] = []

	if args.files:
		for pattern in args.files:
			# Use glob to find files matching the pattern
			# Important: Ensure glob pattern is interpreted correctly by the shell or Python
			# Using pathlib's rglob for potentially simpler recursive searching if needed.
			# Simple glob.glob should work for patterns like "source/**/*.cpp"
			try:
				found_files = glob.glob(pattern, recursive=True)
				if not found_files:
					print(f"Warning: Pattern '{pattern}' did not match any files.", file=sys.stderr)
				else:
					files_to_check.extend(Path(f) for f in found_files if Path(f).is_file())
			except Exception as e:
				print(f"Error processing glob pattern '{pattern}': {e}", file=sys.stderr)
				sys.exit(1)

	else:
		# Default paths if no arguments are given
		default_patterns = [
			'source/**/*.cpp', 'source/**/*.h',
			'tests/**/*.cpp', 'tests/**/*.h'
		]
		print("No specific files provided, checking default paths...")
		for pattern in default_patterns:
			try:
				found_files = glob.glob(pattern, recursive=True)
				files_to_check.extend(Path(f) for f in found_files if Path(f).is_file())
			except Exception as e:
				print(f"Error processing default glob pattern '{pattern}': {e}", file=sys.stderr)
				# Continue with other defaults if one fails
				
	# Remove duplicates and sort files for consistent processing order
	files_to_check = sorted(list(set(files_to_check)))

	if not files_to_check:
		print("No files found to check.", file=sys.stderr)
		sys.exit(0)

	print(f"Checking {len(files_to_check)} files...")

	files_with_issues = 0
	for file_path in files_to_check:
		try:
			# Read file content, keeping original line endings
			with open(file_path, "r", encoding='ascii', newline='') as f:
				# Use newline='' to disable universal newlines, preserving CRLF/CR for checking
				# Specify ascii encoding as required by check_line_format
				file_content = f.readlines()
		except FileNotFoundError:
			print(f"\nError: File not found: {file_path}", file=sys.stderr)
			total_errors += 1 # Count this as an error
			continue
		except UnicodeDecodeError:
			print(f"\nFile: {file_path}")
			print("\tERROR: line 1: File is not valid ASCII.")
			total_errors += 1
			files_with_issues += 1
			continue
		except Exception as e:
			print(f"\nError reading file {file_path}: {e}", file=sys.stderr)
			total_errors += 1 # Count this as an error
			continue

		# Perform the checks
		errors_found, warnings_found = check_code_style(file_path, file_content)

		if errors_found or warnings_found:
			files_with_issues += 1
			print(f"\nFile: {file_path}")
			# Print sorted errors and warnings
			if errors_found:
				print(*errors_found, sep='\n')
				total_errors += len(errors_found)
			if warnings_found:
				print(*warnings_found, sep='\n')
				total_warnings += len(warnings_found)

	# --- Final Summary ---
	print("\n--- Check Summary ---")
	if total_errors > 0:
		error_plural = "error" if total_errors == 1 else "errors"
		warning_plural = "warning" if total_warnings == 1 else "warnings"
		if total_warnings > 0:
			print(f"Found {total_errors} formatting {error_plural} and {total_warnings} {warning_plural} in {files_with_issues} file(s).")
		else:
			print(f"Found {total_errors} formatting {error_plural} in {files_with_issues} file(s).")
		sys.exit(1) # Exit with error code
	elif total_warnings > 0:
		warning_plural = "warning" if total_warnings == 1 else "warnings"
		print(f"Found {total_warnings} formatting {warning_plural} in {files_with_issues} file(s). No errors.")
		sys.exit(0) # Exit with success code, but indicate warnings were found
	else:
		print(f"No formatting errors or warnings found in {len(files_to_check)} file(s) checked.")
		sys.exit(0) # Exit with success code

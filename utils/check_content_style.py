#!/usr/bin/python
# check_content_style.py
# Copyright (c) 2023 by tibetiroka
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
import os.path
import sys

import regex as re
import json


# A class representing the result of a formatting check.
# 'errors' and 'warnings' are lists of Error and Warning objects, respectively.
# 'fixed_errors' and 'fixed_warnings' are similar, but for fixed issues, instead of unfixable ones.
# 'new_file_contents' is a string list of each line, without line separators. It is possible for this list to not be empty, even if there are no fixed errors / warnings.
class CheckResult(object):
	def __init__(self, errors=None, warnings=None, fixed_errors=None, fixed_warnings=None, new_file_contents=None):
		self.fixed_errors = fixed_errors
		self.fixed_warnings = fixed_warnings
		self.errors = errors
		self.warnings = warnings
		self.new_file_contents = new_file_contents
		if new_file_contents is None:
			self.new_file_contents = []
		if warnings is None:
			self.warnings = []
		if errors is None:
			self.errors = []
		if fixed_warnings is None:
			self.fixed_warnings = []
		if fixed_errors is None:
			self.fixed_errors = []

	# Checks whether the file's contents should be modified and reloaded after this check.
	# Takes no parameters.
	# Return value: true if the file was changed, false otherwise.
	def should_reload(self):
		return len(self.fixed_errors) > 0 or len(self.fixed_warnings) > 0

	# Combines the results of this CheckResult with the other CheckResult.
	# Parameters:
	# result: the other CheckResult
	# Has no return value.
	def combine_with(self, result):
		self.fixed_errors += result.fixed_errors
		self.fixed_warnings += result.fixed_warnings
		self.errors += result.errors
		self.warnings += result.warnings
		self.new_file_contents = result.new_file_contents


# A class representing error messages.
# line: the current line number
# reason: the reason for the error
class Error(object):

	def __init__(self, line, reason):
		self.line = line
		self.reason = reason

	def __str__(self):
		return f"\tERROR: line {self.line}: {self.reason}"

	def __lt__(self, other):
		return self.line < other.line

	def __eq__(self, other):
		return self.line == other.line and self.reason == other.reason

	def __hash__(self):
		return self.line


# A class representing warning messages.
# line: the current line number
# reason: the reason for the warning
class Warning(Error):

	def __init__(self, line, reason):
		Error.__init__(self, line, reason)

	def __str__(self):
		return f"\tWARNING: line {self.line}: {self.reason}"


# Prints a help message. This is used when the --help option is given.
# Accepts no parameters and has no return value.
def print_help():
	help_message = [
		["Data file checker script written for Endless Sky by tibetiroka."],
		["Usage: check_content_style [OPTION]... [--files FILE... | --add-files FILE...]"],
		["Checks that text files follow the configured formatting rules."],
		[],
		["Options:"],
		["", "-h", "--help", "Display this help message and exit."],
		["", "-c", "--config-help", "Display a help message for the configuration file and exit."],
		["", "-a", "--auto-correct", "Attempts to automatically correct formatting issues. This is not supported in all cases. Use with caution."],
		["", "-n", "--no-correct", "Does not correct formatting issues. This is the default option."],
		["", "-f", "--format-file [file]", "Specifies the location of the JSON file with the formatting rules. The default value is './utils/contentStyle.json'."],
		["", "-R", "--no-recursion", "Do not look for files recursively. Please note that some pathname patterns are implicitly recursive, and are not disabled with this option."],
		["", "-r", "--recursive", "Look for files recursively. This is the default option. Please note that some pathname patterns are implicitly non-recursive, and are not recursively expanded with this option."],
		[],
		["After these options, the --files or the --add-files option can be passed. Any further argument should be a file name, that is later added to the list of data roots. Pathname patterns are supported."],
		["The --files option specifies the list of files or directories where the style checks are performed. This option overrides the 'dataRoots' entry of the configuration file. Pathname patterns are supported."],
		["The --add-files option is similar, but it appends to the 'dataRoots' entry, instead of replacing it."],
		[],
		["Exit codes:"],
		["", "0", "Successful execution and no formatting errors found."],
		["", "1", "Unknown error."],
		["", "2", "Configuration file is not available or is not properly formatted."],
		["", "3", "An unknown option was passed via command line."],
		["", "4", "Successful execution, but there are remaining formatting errors."],
	]
	for row in help_message:
		print("{:<4} {:<2} {:<20} {:<}".format(*[*row, "", "", "", ""]).rstrip())


# Prints a help message. This is used when the --config-help option is given.
# Accepts no parameters and has no return value.
def print_config_help():
	help_message = [
		["The configuration file should follow the JSON format. Please note that only the 'rules' object is exposed when formatting scripts; any top-level entries are used only to filter files."],
		[],
		["Entries:"],
		["", "dataRoots", "An array of files or folders that contain data files. Pathname patterns are supported."],
		["", "rules", "The formatting rules, as a JSON object. The object contains the following entries:"],
		["", "", "", "indentation", "A string representing a single level of indentation."],
		["", "", "", "maxIndentationIncrease", "The maximum number of indentation levels that can be added per line. It is recommended to set this value to 1."],
		["", "", "", "indentEmptyLines", "Whether to indent empty lines to the level of the next non-empty line. Value should be \"never\", \"always\" or \"either\"."],
		["", "", "", "forceUnixLineSeparator", "Whether to force all line separators to be Unix-style. An error is produced for each invalid line separator."],
		["", "", "", "trailingEmptyLine", "Whether to have a trailing empty line at the end of every file. Value should be \"never\", \"always\" or \"either\"."],
		["", "", "", "checkCopyright", "Whether to verify the copyright header of each file."],
		["", "", "", "copyrightBlacklist", "An array of files that are excluded from the copyright check."],
		["", "", "", "copyrightFormats", "An array of supported copyright formats. Each format is a JSON object with the following entries:"],
		["", "", "", "", "", "holder", "A regex matching the entire 'copyright holder' line. This is repeatedly matched to the beginning of the file."],
		["", "", "", "", "", "notice", "An array of regexes matching each subsequent line of the copyright notice."],
		["", "", "", "regexChecks", "An array of regex-based checks that are applied to individual lines. The checks are grouped by the lines they are applied to. Each entry is a JSON object with the following entries:"],
		["", "", "", "", "", "excludedNodes", "An array of regexes matching data nodes that the checks are not applied to. Indentation is not taken into account. Defaults to an empty array."],
		["", "", "", "", "", "excludeComments", "Whether to exclude comments. Defaults to true."],
		["", "", "", "", "", "excludeKeywords", "Whether to exclude keyword lines. Defaults to true."],
		["", "", "", "", "", "checks", "An array of regex checks. Each entry is a JSON object with the following entries:"],
		["", "", "", "", "", "", "", "description", "The description of the check. This is displayed when it is found in a file."],
		["", "", "", "", "", "", "", "regex", "The regex matching a formatting issue in a line."],
		["", "", "", "", "", "", "", "except", "An array of regular expressions. If any of these match the match result of 'regex', the check is discarded. Defaults to an empty array."],
		["", "", "", "", "", "", "", "isError", "Whether the formatting issue should be marked as an error or a warning. Defaults to true if not specified."],
		["", "", "", "", "", "", "", "correction", "A JSON object specifying how to automatically correct this issue. If not specified, the issue cannot be corrected. Entries:"],
		["", "", "", "", "", "", "", "", "", "parseEntireLine", "Whether to edit the entire line, or only the segment where the regex matched. Defaults to false."],
		["", "", "", "", "", "", "", "", "", "matchReplacement", "A regex to match the part of the line that should be replaced. Defaults to the previously defined regex."],
		["", "", "", "", "", "", "", "", "", "replaceWith", "The replacement value for the line. Supports backslash regex substitutions."],
	]
	for row in help_message:
		print("{:<4} {:<15} {:<4} {:<25} {:<4} {:<15} {:<4} {:<15} {:<4} {:<20} {:<}".format(*[*row, "", "", "", "", "", "", "", "", "", "", ""]).rstrip())


# Loads the configuration file.
# Parameters:
# config_file: the (string) pathname of the config file
# Returns: The config file as a dict, or none if it cannot be loaded
def load_config(config_file):
	try:
		return json.load(open(config_file, "r"))
	except Exception as e:
		print("Could not load configuration file:")
		print(e)


# Checks that the specified text uses unix-style line endings. Returns immediately if this check is not enabled in the configuration.
# Parameters:
# contents: the contents of the file
# auto_correct: whether to attempt to correct the issue
# config: the script configuration
# Return value: a CheckResult
def check_line_separators(contents, auto_correct, config):
	result = CheckResult()

	if config["forceUnixLineSeparator"]:
		for index, line in enumerate(contents):
			if line.endswith("\r\n"):
				result.errors.append(Error(index + 1, "line separators should use LF only; found CRLF"))
			elif line.endswith("\r"):
				result.errors.append(Error(index + 1, "line separators should use LF only; found CR"))

	if config["trailingEmptyLine"] == "always" and not (contents[-1].endswith('\r') or contents[-1].endswith('\n')):
		result.errors.append(Error(len(contents), "missing trailing empty line"))
	elif config["trailingEmptyLine"] == "never" and (contents[-1].endswith('\r') or contents[-1].endswith('\n')):
		result.errors.append(Error(len(contents), "trailing empty line"))

	if auto_correct:
		result.new_file_contents = [line.removesuffix('\n').removesuffix('\r') for line in contents]
		if config["trailingEmptyLine"] == "always":
			result.new_file_contents.append("")
		elif config["trailingEmptyLine"] == "either":
			result.new_file_contents[-1] = contents[-1]
		result.fixed_errors += result.errors
		result.fixed_errors += result.warnings
		result.errors = []
		result.warnings = []

	return result


# Checks the copyright header of the file. Please note that copyright headers cannot be automatically corrected.
# Parameters:
# contents: the contents of the file
# auto_correct: whether to attempt to correct the issue
# config: the script configuration
# Return value: a CheckResult
def check_copyright(contents, auto_correct, config):
	if config["checkCopyright"]:
		for copyright_format in config["copyrightFormats"]:
			holder = copyright_format["holder"]
			notice = copyright_format["notice"]

			# Matching copyright holder line(s)
			text_index = 0
			for line in contents:
				if re.fullmatch(holder, line) is not None:
					text_index += 1
				else:
					break

			# Couldn't match first line
			if text_index == 0:
				continue

			# Checking the remaining lines
			success = True
			for i, (expected, actual) in enumerate(zip(notice, contents[text_index:])):
				if re.fullmatch(expected, actual) is None:
					success = False
					break
			# Match
			if success:
				return CheckResult()
		# No format matched
		return CheckResult([Error(1, "invalid copyright header")])
	return CheckResult()


# Counts the level of indentation for this line of text.
# Parameters:
# text: the text to count indents in
# Returns: the number of leading tabulators
def count_indent(indent, text):
	count = 0
	while text.startswith(indent):
		text = text.removeprefix(indent)
		count += 1
	return count


# Checks the indentation of each line in the file.
# Parameters:
# contents: the contents of the file
# auto_correct: whether to attempt to correct the issue
# config: the script configuration
# Return value: a CheckResult
def check_indentation(contents, auto_correct, config):
	# Gets how much indentation an empty line should have.
	# Parameters:
	# indent: the type of the indentation
	# index: the index of the empty line
	# lines: the lines of text
	def get_expected_indent(indent, index, lines):
		for i in range(index + 1, len(lines)):
			if not (lines[i].isspace() or lines[i] == ""):
				return count_indent(indent, lines[i])
		return 0

	result = CheckResult()
	result.new_file_contents = [line for line in contents]

	max_delta = config["maxIndentationIncrease"]
	indent = config["indentation"]
	error_if_indent = config["indentEmptyLines"] == "never"
	error_if_no_indent = config["indentEmptyLines"] == "always"

	previous_level = 0
	for index, line in enumerate(contents):
		level = count_indent(indent, line)
		if level - previous_level > max_delta or (level > previous_level and level > get_expected_indent(indent, index - 2, contents) and level > get_expected_indent(indent, index, contents) and line.isspace()):
			# Too much indentation
			# This always is checked, even for empty lines, no matter what's configured
			# In fact, empty lines are not allowed to have more indentation than the previous line.
			warning = Warning(index + 1, "over-indented line")
			if auto_correct:
				if line.isspace():
					for i in range(level - previous_level):
						result.new_file_contents[index] = result.new_file_contents[index].removeprefix(indent)
				else:
					for i in range(level - previous_level - max_delta):
						result.new_file_contents[index] = result.new_file_contents[index].removeprefix(indent)
				line = result.new_file_contents[index]
				result.fixed_warnings.append(warning)
			else:
				result.warnings.append(warning)
		if line.isspace() or line == "":
			# Not enough indentation - only checked on empty lines
			if error_if_indent:
				# No indent on empty lines
				if line != "":
					warning = Warning(index + 1, "indented empty line")
					if auto_correct:
						result.new_file_contents[index] = ""
						result.fixed_warnings.append(warning)
					else:
						result.warnings.append(warning)
			elif error_if_no_indent:
				expected = get_expected_indent(indent, index, contents)
				if level != expected:
					warning = Warning(index + 1, "incorrect indentation on empty line")
					if auto_correct:
						result.new_file_contents[index] = indent * expected + result.new_file_contents[index].lstrip()
						result.fixed_warnings.append(warning)
					else:
						result.warnings.append(warning)
		elif not line.lstrip().startswith("#"):
			previous_level = count_indent(indent, line)
	return result


# Uses the specified list of regexes to find formatting issues.
# Parameters:
# check_group: the group of checks to execute
# contents: the contents of the file
# auto_correct: whether to attempt to correct the issue
# config: the script configuration
# Return value: a CheckResult
def check_with_regex(check_group, contents, auto_correct, config):
	result = CheckResult()
	result.new_file_contents = [line for line in contents]

	regex_list = check_group["checks"]
	for entry in regex_list:
		for index, line in enumerate(contents):
			# SKip filtered lines
			if line is None:
				continue
			# Looking for issues
			if re.search(entry["regex"], line) is not None:
				# Checking exceptions
				if "except" in entry:
					match = re.search(entry["regex"], line)
					match_text = match.string[match.start():match.end()]
					valid = True
					for exception in entry["except"]:
						if re.search(exception, match_text):
							valid = False
							break
					if not valid:
						continue
				# Formatting issue found
				is_error = True if "isError" not in entry else entry["isError"]
				if auto_correct and "correction" in entry:
					# Fixing issue
					fix = entry["correction"]

					use_entire_line = True if "parseEntireLine" not in fix else fix["parseEntireLine"]
					match_regex = entry["regex"] if "matchReplacement" not in fix else fix["matchReplacement"]
					replace_with = fix["replaceWith"]

					# Getting match text
					big_match = re.search(entry["regex"], line)
					big_match_text = big_match.string[big_match.start():big_match.end()]

					# Replacing
					result.new_file_contents[index] = re.sub(match_regex, replace_with, line if use_entire_line else big_match_text)
					if is_error:
						result.fixed_errors.append(Error(index + 1, entry["description"]))
					else:
						result.fixed_warnings.append(Warning(index + 1, entry["description"]))
				else:
					if is_error:
						result.errors.append(Error(index + 1, entry["description"]))
					else:
						result.warnings.append(Warning(index + 1, entry["description"]))
	return result


# Finds lines that are not inside excluded nodes and contain text.
# Parameters:
# contents: the contents of the file
# config: the script configuration rules.
# excluded_nodes: the list of regexes that exclude nodes
# exclude_comments: whether to exclude comments
# exclude_keywords: whether to exclude keyword lines
# Return value: the entries of 'contents' that contain text
def find_text_lines(contents, config, excluded_nodes, exclude_comments, exclude_keywords):
	new_contents = []

	indent = config["indentation"]
	is_word = False
	word_indent_level = 0

	for line in contents:
		if ('#' in line and exclude_comments):
			line = line.split("#")[0].strip()
		if line == "" or line.isspace():
			new_contents.append(None)
			# Comment or empty line
			continue
		if is_word:
			if count_indent(indent, line) <= word_indent_level:
				is_word = False
		if not is_word:
			stripped = line.strip().split("#")[0]
			for node in excluded_nodes:
				if re.search(node, stripped) is not None:
					is_word = True
					word_indent_level = count_indent(indent, line)
					break
		if not is_word and ("\"" in line or "`" in line or (not exclude_keywords)):
			new_contents.append(line)
		elif line.strip().startswith("#"):
			new_contents.append(line)
		else:
			new_contents.append(None)
	return new_contents


# Rewrites the file with the specified contents. If the configuration forces unix line separators, then that is used. Otherwise, it uses the system default.
# Parameters:
# file: the (string) pathname of the file
# contents: a string list containing each line of the file
# config: the script configuration rules.
def rewrite(file, contents, config):
	# Adding newlines to all but the last line
	new_contents = [line + "\n" for line in contents[:-1]]
	new_contents.append(contents[-1])

	if config["forceUnixLineSeparator"]:
		f = open(file, "w", newline='\n')
	else:
		f = open(file, "w")
	f.writelines(new_contents)
	f.close()


# Checks the file for formatting errors.
# Parameters:
# file: the (string) pathname of the file
# auto_correct: whether to attempt to correct the issue
# config: the script configuration rules. This is the 'rules' object of the original configuration.
# Return value: a CheckResult
def check_content_style(file, auto_correct, config):
	fixed_errors = []
	fixed_warnings = []

	def do_reload():
		nonlocal fixed_errors
		nonlocal fixed_warnings
		nonlocal issues
		fixed_errors += issues.fixed_errors
		fixed_warnings += issues.fixed_warnings
		rewrite(file, issues.new_file_contents, config)

	while True:
		f = open(file, "r", newline='')
		contents = f.readlines()
		f.close()

		# Empty file
		if not contents:
			return CheckResult()

		# Checking line separators
		issues = check_line_separators(contents, auto_correct, config)
		if issues.should_reload():
			do_reload()
			continue

		# Removing line separators
		contents = [line.replace("\r", "").replace("\n", "") for line in contents]

		# Checking copyright
		if file not in config["copyrightBlacklist"]:
			issues.combine_with(check_copyright(contents, auto_correct, config))
			if issues.should_reload():
				do_reload()
				continue

		# Checking indentation
		issues.combine_with(check_indentation(contents, auto_correct, config))
		if issues.should_reload():
			do_reload()
			continue

		for check_group in config["regexChecks"]:
			excluded_nodes = [] if "excludedNodes" not in check_group else check_group["excludedNodes"]
			exclude_comments = True if "excludeComments" not in check_group else check_group["excludeComments"]
			exclude_keywords = True if "excludeKeywords" not in check_group else check_group["excludeKeywords"]

			restricted_contents = find_text_lines(contents, config, excluded_nodes, exclude_comments, exclude_keywords)
			issues.combine_with(check_with_regex(check_group, restricted_contents, auto_correct, config))
			if issues.should_reload():
				# Prevent deleting filtered lines
				issues.new_file_contents = [(line if line is not None else contents[index]) for (index, line) in enumerate(issues.new_file_contents)]

				do_reload()
				break
		else:
			# All done
			issues.fixed_errors = fixed_errors
			issues.fixed_warnings = fixed_warnings
			return issues
	return CheckResult()


# Prints the result of the style checks for the specified file.
# Parameters:
# file: the (string) pathname of the file
# result: the CheckResult for the file
# No return value. The formatted message is written to the output.
def print_file_result(file, result):
	if result.errors or result.warnings:
		print(file)
		result.errors.sort()
		result.warnings.sort()
		if result.errors:
			print(*result.errors, sep='\n')
		if result.warnings:
			print(*result.warnings, sep='\n')


# Prints the overall result of the style checks.
# Parameters:
# fixed_errors: the number of fixed errors
# fixed_warnings: the number of fixed warnings
# errors: the number of errors (excluding fixed ones)
# warnings: the number of warnings (excluding fixed ones)
# No return value. The formatted message is written to the output.
def print_result(fixed_errors, fixed_warnings, errors, warnings):
	text = ""
	if fixed_errors > 0 or fixed_warnings > 0:
		text += "Fixed "
		if fixed_errors > 0:
			text += str(fixed_errors) + " " + ("error" if fixed_errors == 1 else "errors")
		if fixed_warnings > 0:
			if fixed_errors > 0:
				text += " and "
			text += str(fixed_warnings) + " " + ("warning" if fixed_warnings == 1 else "warnings")
		text += ". "
	if errors == 0 and warnings == 0:
		text += "No " + ("further " if fixed_errors > 0 or fixed_warnings > 0 else "") + "issues found."
	else:
		text += "Found "
		if errors > 0:
			text += str(errors) + " " + ("error" if errors == 1 else "errors")
		if warnings > 0:
			if errors > 0:
				text += " and "
			text += str(warnings) + " " + ("warning" if warnings == 1 else "warnings")
		if fixed_errors > 0 or fixed_warnings > 0:
			text += " that could not be fixed"
		text += "."
	print(text)


if __name__ == '__main__':
	auto_correct = False
	format_file = "./utils/contentStyle.json"
	recursive = True
	add_files = True
	# Processing command line arguments.
	# The --files option is processed later.
	resume_index = 1
	for i in range(1, len(sys.argv)):
		arg = sys.argv[i]
		if arg == "-h" or arg == "--help":
			print_help()
			exit(0)
		elif arg == "-c" or arg == "--config-help":
			print_config_help()
			exit(0)
		elif arg == "-a" or arg == "--auto-correct":
			auto_correct = True
		elif arg == "-n" or arg == "--no-correct":
			auto_correct = False
		elif arg == "-R" or arg == "--no-recursion":
			recursive = False
		elif arg == "-r" or arg == "--recursive":
			recursive = True
		elif arg == "-f" or arg == "--format-file":
			if len(sys.argv) > i + 1:
				format_file = sys.argv[i + 1]
				i = i + 1
		elif arg == "--files":
			resume_index = i + 1
			add_files = False
			break
		elif arg == "--add-files":
			resume_index = i + 1
			break
		else:
			print("Unknown option '" + sys.argv[i] + "'")
			exit(3)
	# loading config file
	config = load_config(format_file)
	if config is None:
		exit(2)

	# Adding input files
	if add_files:
		config["dataRoots"] += sys.argv[resume_index:]
	else:
		config["dataRoots"] = sys.argv[resume_index:]

	# Listing data files
	data_files = []
	for root in config["dataRoots"]:
		# Adds all listed paths if:
		# - it's a file, and
		# - has an accepted extension
		data_files += [file for file in glob.glob(root, recursive=recursive) if os.path.isfile(file)]
	# Removing duplicates and sorting
	# Since not all path names are resolved, there could still be duplicate entries after this
	data_files = list(set(data_files))
	data_files.sort()

	# Parsing files
	fixed_error_count = 0
	fixed_warning_count = 0
	error_count = 0
	warning_count = 0

	for file in data_files:
		result = check_content_style(file, auto_correct, config["rules"])

		fixed_error_count += len(result.fixed_errors)
		fixed_warning_count += len(result.fixed_warnings)
		error_count += len(result.errors)
		warning_count += len(result.warnings)

		print_file_result(file, result)

	print_result(fixed_error_count, fixed_warning_count, error_count, warning_count)

	exit(4 if error_count > 0 else 0)

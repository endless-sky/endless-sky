#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# check_content_style.py
# Copyright (c) 2023 by tibetiroka
# Enhanced and upgraded in 2025.
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
import json
import logging
import sys
from pathlib import Path
from typing import List, Optional, Dict, Any, Tuple, Union

# Use the external 'regex' library if needed for advanced features,
# otherwise fallback to standard 're'. For this script, 're' might suffice.
# Let's try with standard 're' first and see if anything breaks.
# If specific 'regex' features were truly needed, switch back.
import re
# import regex as re # Uncomment this and comment above if 'regex' is required

# --- Configuration Constants ---
DEFAULT_CONFIG_PATH = Path("./utils/contentStyle.json")
ENCODING = "utf-8"

# --- Mode Constants ---
MODE_NEVER = "never"
MODE_ALWAYS = "always"
MODE_EITHER = "either"
VALID_MODES = {MODE_NEVER, MODE_ALWAYS, MODE_EITHER}

# --- Logging Setup ---
logging.basicConfig(level=logging.INFO, format='%(message)s')
log = logging.getLogger(__name__)

# --- Data Classes ---

class BaseIssue:
    """Base class for representing issues found during checks."""
    def __init__(self, line: int, reason: str):
        """
        Args:
            line (int): The 1-based line number where the issue occurred.
            reason (str): A description of the issue.
        """
        self.line = line
        self.reason = reason

    def __str__(self) -> str:
        # Overridden by subclasses
        return f"line {self.line}: {self.reason}"

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}(line={self.line}, reason='{self.reason}')"

    def __lt__(self, other: 'BaseIssue') -> bool:
        return self.line < other.line

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, BaseIssue):
            return NotImplemented
        return self.line == other.line and self.reason == other.reason

    def __hash__(self) -> int:
        # Hash primarily based on line for sorting/grouping,
        # though reason makes it unique.
        return hash((self.line, self.reason))

class Error(BaseIssue):
    """Represents an error message."""
    def __str__(self) -> str:
        return f"\tERROR: line {self.line}: {self.reason}"

class Warning(BaseIssue):
    """Represents a warning message."""
    def __str__(self) -> str:
        return f"\tWARNING: line {self.line}: {self.reason}"

class CheckResult:
    """
    Represents the result of formatting checks for a file.

    Attributes:
        errors (List[Error]): List of unfixed errors found.
        warnings (List[Warning]): List of unfixed warnings found.
        fixed_errors (List[Error]): List of errors that were automatically fixed.
        fixed_warnings (List[Warning]): List of warnings that were automatically fixed.
        new_file_contents (Optional[List[str]]): The modified lines of the file
            (without line separators) if auto-correction occurred. None otherwise.
    """
    def __init__(self):
        self.errors: List[Error] = []
        self.warnings: List[Warning] = []
        self.fixed_errors: List[Error] = []
        self.fixed_warnings: List[Warning] = []
        self.new_file_contents: Optional[List[str]] = None

    def add_error(self, line: int, reason: str):
        self.errors.append(Error(line, reason))

    def add_warning(self, line: int, reason: str):
        self.warnings.append(Warning(line, reason))

    def add_fixed_error(self, line: int, reason: str):
        self.fixed_errors.append(Error(line, reason))

    def add_fixed_warning(self, line: int, reason: str):
        self.fixed_warnings.append(Warning(line, reason))

    def set_new_contents(self, contents: List[str]):
        self.new_file_contents = contents

    def should_reload(self) -> bool:
        """Checks if the file content was modified and needs reloading."""
        return self.new_file_contents is not None

    def clear_unfixed_issues(self):
        """Moves unfixed issues to fixed lists. Call after applying fixes."""
        self.fixed_errors.extend(self.errors)
        self.fixed_warnings.extend(self.warnings)
        self.errors = []
        self.warnings = []

    def combine_with(self, other: 'CheckResult'):
        """Combines the results from another CheckResult into this one."""
        self.errors.extend(other.errors)
        self.warnings.extend(other.warnings)
        self.fixed_errors.extend(other.fixed_errors)
        self.fixed_warnings.extend(other.fixed_warnings)
        # If the other result modified contents, take theirs.
        if other.new_file_contents is not None:
            self.new_file_contents = other.new_file_contents

    def has_issues(self) -> bool:
        """Returns True if there are any unfixed issues."""
        return bool(self.errors or self.warnings)

    def sort_issues(self):
        """Sorts all issue lists by line number."""
        self.errors.sort()
        self.warnings.sort()
        self.fixed_errors.sort()
        self.fixed_warnings.sort()

    def __repr__(self) -> str:
        return (f"CheckResult(errors={len(self.errors)}, warnings={len(self.warnings)}, "
                f"fixed_errors={len(self.fixed_errors)}, fixed_warnings={len(self.fixed_warnings)}, "
                f"modified={self.new_file_contents is not None})")

# --- Helper Functions ---

def print_help_message():
    """Prints a detailed help message (similar to original)."""
    help_text = """
Data file checker script written for Endless Sky by tibetiroka.
Enhanced and upgraded in 2025.

Usage: check_content_style [OPTIONS] [--files FILE... | --add-files FILE...]

Checks that text files follow the configured formatting rules.

Options:
  -h, --help            Display this help message and exit.
  --config-help         Display help for the configuration file format and exit.
  -a, --auto-correct    Attempt to automatically correct formatting issues.
                        Use with caution. This modifies files in place.
  -n, --no-correct      Do not correct formatting issues (default).
  -f CONFIG_FILE, --format-file CONFIG_FILE
                        Specify the location of the JSON configuration file.
                        Default: ./utils/contentStyle.json
  -R, --no-recursion    Do not search for files recursively in specified directories.
  -r, --recursive       Search for files recursively (default).

File Specification (Required):
  Use *one* of the following options followed by file/directory paths or glob patterns:
  --files FILE_OR_PATTERN [FILE_OR_PATTERN ...]
                        Specify the exact list of files/directories to check.
                        Overrides 'dataRoots' in the config file.
  --add-files FILE_OR_PATTERN [FILE_OR_PATTERN ...]
                        Append specified files/directories to 'dataRoots'
                        from the config file.

Exit Codes:
  0: Successful execution and no formatting errors found.
  1: Unknown error during execution.
  2: Configuration file error (not found, invalid JSON, missing keys).
  3: Invalid command-line arguments provided.
  4: Successful execution, but unfixed formatting errors remain.
"""
    print(help_text) # Using print directly for help message is fine

def print_config_help_message():
    """Prints help for the configuration file format (similar to original)."""
    config_help_text = """
Configuration File Format (JSON):

The configuration file defines the data roots and formatting rules.

Top-Level Entries:
  "dataRoots": [ "path/or/pattern1", ... ] (Optional)
    An array of file paths, directory paths, or glob patterns identifying
    the files/folders to check by default. Can be overridden or extended
    via command-line arguments.

  "rules": { ... } (Required)
    An object containing the specific formatting rules.

Rules Object Entries:
  "indentation": "string" (e.g., "\\t" or "    ")
    The string representing a single level of indentation.

  "maxIndentationIncrease": integer (e.g., 1)
    Maximum number of indentation levels allowed to increase per line.
    Recommended value: 1.

  "indentEmptyLines": "never" | "always" | "either"
    Rule for indenting empty lines.
      "never": Empty lines must not be indented.
      "always": Empty lines must be indented to the level of the next
                non-empty line.
      "either": Both indented and unindented empty lines are allowed.

  "forceUnixLineSeparator": boolean (true/false)
    If true, enforces LF line endings (\\n) and flags CRLF (\\r\\n) or CR (\\r).

  "trailingEmptyLine": "never" | "always" | "either"
    Rule for the presence of a single empty line at the end of the file.

  "checkCopyright": boolean (true/false)
    If true, verifies the copyright header at the beginning of files.

  "copyrightBlacklist": [ "file/path1", ... ] (Optional)
    An array of file paths (relative to the project root or absolute)
    to exclude from the copyright check.

  "copyrightFormats": [ { "holder": "regex", "notice": ["regex", ...] }, ... ]
    An array of supported copyright formats. Each format requires:
      "holder": A regex matching the entire copyright holder line(s).
                Matched repeatedly from the start of the file.
      "notice": An array of regexes, each matching a subsequent required
                line of the copyright notice.

  "regexChecks": [ { "excludedNodes": [...], "excludeComments": ..., ... }, ... ]
    An array of regex check groups. Each group object contains:
      "excludedNodes": [ "regex", ... ] (Optional, default: [])
        Regexes matching data node definitions (indentation ignored).
        Lines within these nodes are excluded from checks in this group.
      "excludeComments": boolean (Optional, default: true)
        If true, exclude comment lines (starting with '#') from checks.
      "excludeKeywords": boolean (Optional, default: true)
        If true, exclude lines considered 'keyword lines' (heuristic) from checks.
      "checks": [ { "description": "...", "regex": "...", ... }, ... ]
        An array of individual regex checks within this group. Each check requires:
          "description": "string"
            Message displayed when this issue is found.
          "regex": "regex"
            The regex pattern identifying the formatting issue.
          "except": [ "regex", ... ] (Optional, default: [])
            If any of these regexes match the text found by the main "regex",
            the issue is ignored.
          "isError": boolean (Optional, default: true)
            If true, report as an Error; otherwise, report as a Warning.
          "correction": { ... } (Optional)
            If present, enables auto-correction for this issue. Requires:
              "parseEntireLine": boolean (Optional, default: false)
                If true, the 'replaceWith' applies to the entire line.
                If false, it applies only to the part matched by 'matchReplacement'.
              "matchReplacement": "regex" (Optional, default: same as "regex")
                The regex identifying the specific part to replace within the match
                (or the whole line if parseEntireLine is true).
              "replaceWith": "string"
                The replacement string. Can use regex backreferences (e.g., \\1, \\2).
"""
    print(config_help_text) # Using print directly for help message is fine

def load_config(config_path: Path) -> Optional[Dict[str, Any]]:
    """
    Loads the JSON configuration file.

    Args:
        config_path: Path to the configuration file.

    Returns:
        The loaded configuration as a dictionary, or None if loading fails.
    """
    try:
        with config_path.open("r", encoding=ENCODING) as f:
            config = json.load(f)
        # Basic validation (can be expanded)
        if "rules" not in config or not isinstance(config["rules"], dict):
            log.error(f"Configuration error: Missing or invalid 'rules' object in {config_path}")
            return None
        # Add more validation as needed (e.g., checking types of rule values)
        return config
    except FileNotFoundError:
        log.error(f"Configuration file not found: {config_path}")
        return None
    except json.JSONDecodeError as e:
        log.error(f"Invalid JSON in configuration file {config_path}: {e}")
        return None
    except Exception as e:
        log.error(f"Could not load configuration file {config_path}: {e}")
        return None

def count_indent(indent_char: str, text: str) -> int:
    """Counts the leading indentation levels in a line."""
    count = 0
    while text.startswith(indent_char):
        text = text.removeprefix(indent_char)
        count += 1
    return count

def rewrite_file(file_path: Path, contents: List[str], config: Dict[str, Any]):
    """
    Writes the given contents back to the file.

    Args:
        file_path: Path to the file to write.
        contents: List of strings (lines without separators).
        config: The 'rules' portion of the configuration.
    """
    newline_char = '\n' if config.get("forceUnixLineSeparator", False) else os.linesep
    try:
        with file_path.open("w", encoding=ENCODING, newline=newline_char) as f:
            if contents:
                # Add newline to all but the last line
                f.writelines(line + newline_char for line in contents[:-1])
                f.write(contents[-1]) # Write last line without trailing newline (it might be added based on config)
                # Handle trailing empty line based on config AFTER writing content
                if config.get("trailingEmptyLine") == MODE_ALWAYS:
                     if not contents[-1].endswith(newline_char): # Ensure only one trailing newline
                         f.write(newline_char)
                # If trailingEmptyLine is 'never', we already wrote without trailing newline.
                # If 'either', the state after writing content is kept.
            else:
                # Handle empty file case if necessary (e.g., trailing empty line rule)
                if config.get("trailingEmptyLine") == MODE_ALWAYS:
                    f.write(newline_char)

    except IOError as e:
        log.error(f"Error writing file {file_path}: {e}")
    except Exception as e:
        log.error(f"Unexpected error rewriting file {file_path}: {e}")


def find_text_lines(
    lines: List[str],
    config: Dict[str, Any],
    excluded_nodes: List[str],
    exclude_comments: bool,
    exclude_keywords: bool
) -> List[Optional[str]]:
    """
    Filters lines, returning a list where relevant lines are kept
    and irrelevant lines (based on rules) are replaced with None.

    Args:
        lines: The content of the file (list of strings without separators).
        config: The 'rules' portion of the configuration.
        excluded_nodes: List of regex patterns for nodes to exclude.
        exclude_comments: Whether to exclude comment lines.
        exclude_keywords: Whether to exclude keyword lines.

    Returns:
        A list of the same length as 'lines', containing either the original
        line (if it should be checked) or None (if it should be skipped).
    """
    filtered_lines: List[Optional[str]] = []
    indent = config.get("indentation", "\t") # Default to tab if not specified
    is_in_excluded_node = False
    node_indent_level = 0
    compiled_excluded_nodes = [re.compile(pattern) for pattern in excluded_nodes]

    for line in lines:
        original_line = line # Keep original for potential inclusion
        current_indent_level = count_indent(indent, line)

        # Reset exclusion if indent decreases
        if is_in_excluded_node and current_indent_level <= node_indent_level:
            is_in_excluded_node = False

        # Skip lines inside excluded nodes
        if is_in_excluded_node:
            filtered_lines.append(None)
            continue

        # Handle comments
        line_content = line
        if exclude_comments and '#' in line:
            line_content = line.split("#", 1)[0]

        stripped_content = line_content.strip()

        # Skip empty or whitespace-only lines after comment removal
        if not stripped_content:
            filtered_lines.append(None)
            continue

        # Check if this line starts a new excluded node
        for node_pattern in compiled_excluded_nodes:
            if node_pattern.search(stripped_content):
                is_in_excluded_node = True
                node_indent_level = current_indent_level
                filtered_lines.append(None)
                break
        else: # If no break occurred (not an excluded node start)
            # Heuristic for 'keyword lines' (lines without quotes or backticks?)
            # This might need refinement based on the actual data file format.
            is_keyword_like = "\"" not in line and "`" not in line

            if exclude_keywords and is_keyword_like:
                 # Even if keyword-like, keep if it's a comment line (handled earlier if needed)
                 # or if the original line starts with # (explicit comment)
                 if line.strip().startswith("#"):
                     filtered_lines.append(original_line) # Keep explicit comments if not excluded
                 else:
                     filtered_lines.append(None) # Exclude keyword line
            else:
                filtered_lines.append(original_line) # Keep non-keyword lines or if keywords aren't excluded

    return filtered_lines


# --- Checking Functions ---

def check_line_separators(
    raw_lines: List[str], auto_correct: bool, config: Dict[str, Any]
) -> CheckResult:
    """
    Checks line separators and trailing empty line rules.

    Args:
        raw_lines: List of lines read directly from the file (with separators).
        auto_correct: Whether to attempt corrections.
        config: The 'rules' portion of the configuration.

    Returns:
        A CheckResult object.
    """
    result = CheckResult()
    new_contents: List[str] = []
    issues_found = False

    force_unix = config.get("forceUnixLineSeparator", False)
    trailing_rule = config.get("trailingEmptyLine", MODE_EITHER)
    if trailing_rule not in VALID_MODES:
        log.warning(f"Invalid 'trailingEmptyLine' mode: {trailing_rule}. Defaulting to '{MODE_EITHER}'.")
        trailing_rule = MODE_EITHER

    last_line_index = len(raw_lines) - 1

    for i, line in enumerate(raw_lines):
        line_num = i + 1
        original_line_ending = ""
        content = line

        # Check line endings
        if line.endswith("\r\n"):
            original_line_ending = "\r\n"
            content = line[:-2]
            if force_unix:
                result.add_error(line_num, "Line separators should use LF only; found CRLF")
                issues_found = True
        elif line.endswith("\n"):
            original_line_ending = "\n"
            content = line[:-1]
            # This is the desired state if force_unix is true
        elif line.endswith("\r"):
            original_line_ending = "\r"
            content = line[:-1]
            if force_unix:
                result.add_error(line_num, "Line separators should use LF only; found CR")
                issues_found = True
        # else: no line ending (potentially the last line)

        new_contents.append(content)

        # Check trailing empty line rule (only on the actual last line)
        if i == last_line_index:
            has_trailing_newline = bool(original_line_ending)
            if trailing_rule == MODE_ALWAYS and not has_trailing_newline:
                 result.add_error(line_num, "Missing trailing newline at end of file")
                 issues_found = True
            elif trailing_rule == MODE_NEVER and has_trailing_newline:
                 result.add_error(line_num, "Trailing newline found at end of file")
                 issues_found = True

    if auto_correct and issues_found:
        # Apply corrections to new_contents based on rules
        corrected_contents = list(new_contents) # Work on a copy

        # Trailing newline correction
        if trailing_rule == MODE_ALWAYS:
            # Ensure the file ends with exactly one empty string element if needed
             if not corrected_contents or corrected_contents[-1]: # Add if list is empty or last line has content
                 corrected_contents.append("")
        elif trailing_rule == MODE_NEVER:
            # Remove trailing empty string elements
            while corrected_contents and not corrected_contents[-1]:
                 corrected_contents.pop()

        # Note: Line separator correction happens during rewrite_file based on force_unix

        result.set_new_contents(corrected_contents)
        result.clear_unfixed_issues() # Mark found issues as fixed

    return result


def check_copyright(
    lines: List[str], config: Dict[str, Any], file_path: Path
) -> CheckResult:
    """
    Checks the copyright header of the file.

    Args:
        lines: File content (list of strings without separators).
        config: The 'rules' portion of the configuration.
        file_path: The path to the file being checked (for blacklist).

    Returns:
        A CheckResult object. Copyright issues cannot be auto-corrected by this script.
    """
    result = CheckResult()
    if not config.get("checkCopyright", False):
        return result

    # Check blacklist (use relative paths if possible or absolute)
    blacklist = config.get("copyrightBlacklist", [])
    # Normalize paths for comparison if necessary, or compare as strings
    if str(file_path) in blacklist or file_path.name in blacklist:
         log.debug(f"Skipping copyright check for blacklisted file: {file_path}")
         return result

    copyright_formats = config.get("copyrightFormats", [])
    if not copyright_formats:
        log.warning("Copyright check enabled, but no 'copyrightFormats' defined in config.")
        return result

    if not lines: # Cannot check copyright on empty file
        return result

    for c_format in copyright_formats:
        holder_pattern_str = c_format.get("holder")
        notice_patterns_str = c_format.get("notice", [])

        if not holder_pattern_str:
            log.warning("Skipping invalid copyright format: missing 'holder' regex.")
            continue

        try:
            holder_regex = re.compile(holder_pattern_str)
            notice_regexes = [re.compile(p) for p in notice_patterns_str]
        except re.error as e:
            log.warning(f"Skipping invalid copyright format due to regex error: {e}")
            continue

        # --- Matching Logic ---
        current_line_index = 0
        holder_lines_matched = 0

        # Match holder line(s)
        while current_line_index < len(lines):
            line = lines[current_line_index]
            if holder_regex.fullmatch(line):
                holder_lines_matched += 1
                current_line_index += 1
            else:
                break # Stop matching holder lines

        # If no holder lines matched for this format, try the next format
        if holder_lines_matched == 0:
            continue

        # Check if notice lines match
        format_match = True
        for notice_index, notice_regex in enumerate(notice_regexes):
            file_line_index = current_line_index + notice_index
            if file_line_index >= len(lines) or not notice_regex.fullmatch(lines[file_line_index]):
                format_match = False
                break # This format doesn't match

        # If we matched all holder and notice lines for this format, it's valid
        if format_match:
            log.debug(f"Copyright header matched format in {file_path}")
            return result # Found a valid format

    # If no format matched after checking all of them
    result.add_error(1, "Invalid or missing copyright header")
    return result


def check_indentation(
    lines: List[str], auto_correct: bool, config: Dict[str, Any]
) -> CheckResult:
    """
    Checks indentation rules.

    Args:
        lines: File content (list of strings without separators).
        auto_correct: Whether to attempt corrections.
        config: The 'rules' portion of the configuration.

    Returns:
        A CheckResult object.
    """
    result = CheckResult()
    modified_lines = list(lines)
    issues_found = False

    indent = config.get("indentation", "\t")
    max_delta = config.get("maxIndentationIncrease", 1)
    empty_line_rule = config.get("indentEmptyLines", MODE_EITHER)
    if empty_line_rule not in VALID_MODES:
        log.warning(f"Invalid 'indentEmptyLines' mode: {empty_line_rule}. Defaulting to '{MODE_EITHER}'.")
        empty_line_rule = MODE_EITHER

    def get_expected_indent_for_empty(index: int) -> int:
        """Find indentation level of the next non-empty line."""
        for i in range(index + 1, len(lines)):
            if lines[i].strip(): # Find first non-empty/non-whitespace line
                return count_indent(indent, lines[i])
        return 0 # Default to 0 if no subsequent non-empty line

    previous_level = 0
    for index, line in enumerate(lines):
        line_num = index + 1
        is_empty_or_whitespace = not line.strip()
        current_level = count_indent(indent, line)

        # 1. Check max indentation increase
        # Applied even to empty lines if they are indented more than allowed relative to prev
        if current_level - previous_level > max_delta:
             reason = f"Indentation increased by {current_level - previous_level} levels (max allowed: {max_delta})"
             result.add_warning(line_num, reason)
             issues_found = True
             if auto_correct:
                 # Correct by removing excess indentation
                 excess = current_level - previous_level - max_delta
                 corrected_line = line
                 for _ in range(excess):
                     corrected_line = corrected_line.removeprefix(indent)
                 modified_lines[index] = corrected_line
                 current_level = count_indent(indent, corrected_line) # Update level after fix

        # 2. Check empty line indentation based on rule
        if is_empty_or_whitespace:
            if empty_line_rule == MODE_NEVER and current_level > 0:
                result.add_warning(line_num, "Empty or whitespace-only line should not be indented")
                issues_found = True
                if auto_correct:
                    modified_lines[index] = "" # Remove indentation completely
            elif empty_line_rule == MODE_ALWAYS:
                expected_level = get_expected_indent_for_empty(index)
                if current_level != expected_level:
                    reason = f"Empty line indentation level ({current_level}) does not match expected level ({expected_level})"
                    result.add_warning(line_num, reason)
                    issues_found = True
                    if auto_correct:
                        modified_lines[index] = (indent * expected_level)

        # Update previous_level only for non-empty lines or if empty lines matter for next line's delta
        # Generally, update based on the *content* level, ignoring pure whitespace lines for delta calc.
        if not is_empty_or_whitespace:
             # Ignore comment lines for setting the 'previous level' baseline? Depends on style guide.
             # Original script logic: Update previous_level if not a comment line.
             if not line.lstrip().startswith("#"):
                 previous_level = current_level
        # Else: keep previous_level as it was for checking the next line's delta


    if auto_correct and issues_found:
        result.set_new_contents(modified_lines)
        result.clear_unfixed_issues()

    return result


def check_with_regex(
    check_group: Dict[str, Any],
    lines: List[str],
    auto_correct: bool,
    config: Dict[str, Any]
) -> CheckResult:
    """
    Applies a group of regex-based checks to the relevant lines.

    Args:
        check_group: A dictionary representing a single regex check group from config.
        lines: File content (list of strings without separators).
        auto_correct: Whether to attempt corrections.
        config: The 'rules' portion of the configuration (needed for find_text_lines).

    Returns:
        A CheckResult object.
    """
    result = CheckResult()
    modified_lines = list(lines) # Start with original content
    any_fix_applied = False

    # --- Filter lines based on group rules ---
    excluded_nodes = check_group.get("excludedNodes", [])
    exclude_comments = check_group.get("excludeComments", True)
    exclude_keywords = check_group.get("excludeKeywords", True)

    # Get a list where lines to check are strings, others are None
    target_lines = find_text_lines(lines, config, excluded_nodes, exclude_comments, exclude_keywords)

    # --- Apply checks in the group ---
    for entry in check_group.get("checks", []):
        description = entry.get("description", "Regex check failed")
        regex_str = entry.get("regex")
        except_strs = entry.get("except", [])
        is_error = entry.get("isError", True)
        correction_cfg = entry.get("correction")

        if not regex_str:
            log.warning(f"Skipping regex check with missing 'regex': {description}")
            continue

        try:
            main_regex = re.compile(regex_str)
            except_regexes = [re.compile(e) for e in except_strs]
            match_replacement_regex = None
            replace_with = None
            parse_entire_line = False

            if auto_correct and correction_cfg:
                match_replacement_str = correction_cfg.get("matchReplacement", regex_str)
                replace_with = correction_cfg.get("replaceWith")
                parse_entire_line = correction_cfg.get("parseEntireLine", False)
                if replace_with is None:
                    log.warning(f"Auto-correct configured for '{description}' but 'replaceWith' is missing. Cannot correct.")
                    correction_cfg = None # Disable correction for this check
                else:
                    match_replacement_regex = re.compile(match_replacement_str)

        except re.error as e:
            log.warning(f"Skipping regex check '{description}' due to regex error: {e}")
            continue

        # Iterate through the potentially checkable lines
        for index, line in enumerate(target_lines):
            if line is None: # Skip lines filtered out by find_text_lines
                continue

            line_num = index + 1
            # Use finditer to handle multiple matches on the same line if necessary
            # Though original code used search, let's stick to that unless issues arise.
            match = main_regex.search(line)
            if match:
                match_text = match.group(0) # The text matched by the main regex

                # Check exceptions
                is_excepted = False
                for except_regex in except_regexes:
                    if except_regex.search(match_text):
                        is_excepted = True
                        break
                if is_excepted:
                    continue # Skip this match

                # Issue found, report or fix
                if auto_correct and correction_cfg and match_replacement_regex and replace_with is not not None:
                    # --- Apply Correction ---
                    try:
                         target_text = line if parse_entire_line else match_text
                         # We need to replace within the *modified_lines* list
                         original_line_to_modify = modified_lines[index]

                         if parse_entire_line:
                             # Replace first occurrence in the whole line using match_replacement_regex
                             new_line_content = match_replacement_regex.sub(replace_with, original_line_to_modify, count=1)
                         else:
                             # Replace only the matched part. This is tricky if the matched part
                             # occurs multiple times. Safest is to replace the specific match instance.
                             # Use slicing based on match span on the original line.
                             start, end = match.span()
                             # Apply the replacement logic to the matched segment
                             segment_to_replace = original_line_to_modify[start:end]
                             replaced_segment = match_replacement_regex.sub(replace_with, segment_to_replace, count=1)
                             new_line_content = original_line_to_modify[:start] + replaced_segment + original_line_to_modify[end:]


                         if new_line_content != modified_lines[index]:
                              modified_lines[index] = new_line_content
                              any_fix_applied = True
                              # Add to fixed lists
                              if is_error:
                                  result.add_fixed_error(line_num, description)
                              else:
                                  result.add_fixed_warning(line_num, description)
                         else:
                              # Correction didn't change anything, report as unfixed
                              if is_error: result.add_error(line_num, description)
                              else: result.add_warning(line_num, description)

                    except re.error as e:
                         log.warning(f"Regex error during correction for '{description}' on line {line_num}: {e}")
                         # Report as unfixed if correction failed
                         if is_error: result.add_error(line_num, description)
                         else: result.add_warning(line_num, description)
                    except Exception as e:
                         log.warning(f"Unexpected error during correction for '{description}' on line {line_num}: {e}")
                         # Report as unfixed if correction failed
                         if is_error: result.add_error(line_num, description)
                         else: result.add_warning(line_num, description)

                else:
                    # Report as unfixed issue
                    if is_error:
                        result.add_error(line_num, description)
                    else:
                        result.add_warning(line_num, description)


    if any_fix_applied:
         result.set_new_contents(modified_lines)
         # Don't call clear_unfixed_issues here; fixed issues are added directly above

    return result


# --- Main Orchestration ---

def check_file_content(
    file_path: Path, auto_correct: bool, config: Dict[str, Any]
) -> CheckResult:
    """
    Orchestrates all checks for a single file.

    Args:
        file_path: Path to the file to check.
        auto_correct: Whether to attempt corrections.
        config: The 'rules' portion of the configuration.

    Returns:
        A CheckResult object summarizing all findings for the file.
    """
    overall_result = CheckResult()
    max_iterations = 5 # Prevent potential infinite loops if fixes conflict
    iterations = 0

    while iterations < max_iterations:
        iterations += 1
        log.debug(f"--- Checking {file_path} (Iteration {iterations}) ---")
        current_result = CheckResult()
        file_modified_in_iteration = False

        try:
            # Read raw lines first for line ending checks
            with file_path.open("r", encoding=ENCODING, newline='') as f:
                raw_lines = f.readlines()
        except IOError as e:
            log.error(f"Error reading file {file_path}: {e}")
            current_result.add_error(0, f"Cannot read file: {e}")
            overall_result.combine_with(current_result)
            break # Cannot proceed with this file
        except Exception as e: # Catch potential decoding errors etc.
             log.error(f"Error processing file {file_path}: {e}")
             current_result.add_error(0, f"Cannot process file: {e}")
             overall_result.combine_with(current_result)
             break

        if not raw_lines:
            log.debug(f"File is empty: {file_path}")
            break # No checks needed for empty file (unless trailing newline rule applies - handled?)
            # TODO: Verify how empty files interact with trailing newline rule.

        # --- 1. Line Separators / Trailing Line Check ---
        sep_result = check_line_separators(raw_lines, auto_correct, config)
        current_result.combine_with(sep_result)
        if sep_result.should_reload():
            log.debug(f"Applying line separator/trailing line fixes for {file_path}")
            rewrite_file(file_path, sep_result.new_file_contents, config)
            file_modified_in_iteration = True
            continue # Re-read the file with fixes applied

        # --- Prepare lines without separators for other checks ---
        lines = [line.rstrip('\r\n') for line in raw_lines]

        # --- 2. Copyright Check ---
        # Copyright check happens before indentation/regex, uses cleaned lines
        copy_result = check_copyright(lines, config, file_path)
        current_result.combine_with(copy_result)
        # Copyright check doesn't auto-correct, so no reload needed

        # --- 3. Indentation Check ---
        indent_result = check_indentation(lines, auto_correct, config)
        current_result.combine_with(indent_result)
        if indent_result.should_reload():
            log.debug(f"Applying indentation fixes for {file_path}")
            rewrite_file(file_path, indent_result.new_file_contents, config)
            file_modified_in_iteration = True
            continue # Re-read the file with fixes applied

        # --- 4. Regex Checks ---
        combined_regex_result = CheckResult()
        regex_fixes_applied = False
        current_lines_for_regex = list(lines) # Use current state of lines

        for check_group in config.get("regexChecks", []):
            regex_result = check_with_regex(check_group, current_lines_for_regex, auto_correct, config)
            combined_regex_result.combine_with(regex_result)
            if regex_result.should_reload():
                # A fix was applied within this group, update the lines for the *next* group
                current_lines_for_regex = regex_result.new_file_contents
                regex_fixes_applied = True
                # Don't rewrite yet, wait until all regex groups are processed in this iteration

        current_result.combine_with(combined_regex_result)
        if regex_fixes_applied:
             log.debug(f"Applying regex fixes for {file_path}")
             # Use the final state of lines after all regex groups
             rewrite_file(file_path, current_lines_for_regex, config)
             file_modified_in_iteration = True
             continue # Re-read the file with fixes applied


        # --- End of Iteration ---
        overall_result.combine_with(current_result)
        if not file_modified_in_iteration:
            log.debug(f"No fixes applied in iteration {iterations}. Checks complete for {file_path}.")
            break # No changes in this iteration, stable state reached

    else: # Max iterations reached
        log.warning(f"Maximum check iterations ({max_iterations}) reached for {file_path}. "
                    "There might be conflicting rules or complex fixes.")
        # Report remaining issues from the last attempt
        # Need to re-read the file one last time to get final state for reporting if loop terminated
        try:
            with file_path.open("r", encoding=ENCODING, newline='') as f:
                 final_raw_lines = f.readlines()
            final_lines = [line.rstrip('\r\n') for line in final_raw_lines]
            # Perform one last check pass *without* auto-correct to report remaining issues
            final_check_result = CheckResult()
            final_check_result.combine_with(check_line_separators(final_raw_lines, False, config))
            final_check_result.combine_with(check_copyright(final_lines, config, file_path))
            final_check_result.combine_with(check_indentation(final_lines, False, config))
            for check_group in config.get("regexChecks", []):
                 final_check_result.combine_with(check_with_regex(check_group, final_lines, False, config))
            overall_result.errors = final_check_result.errors # Replace with final unfixed errors
            overall_result.warnings = final_check_result.warnings
        except Exception as e:
             log.error(f"Could not perform final check on {file_path} after max iterations: {e}")


    overall_result.sort_issues()
    return overall_result


def print_file_result(file_path: Path, result: CheckResult):
    """Prints the checking results for a single file."""
    if result.has_issues():
        log.info(f"Issues found in: {file_path}")
        for error in result.errors:
            log.warning(str(error)) # Log errors as warnings in console
        for warning in result.warnings:
            log.info(str(warning)) # Log warnings as info

def print_summary(total_fixed_errors: int, total_fixed_warnings: int,
                  total_errors: int, total_warnings: int):
    """Prints the overall summary of the checks."""
    summary_parts = []
    if total_fixed_errors > 0 or total_fixed_warnings > 0:
        fixed_parts = []
        if total_fixed_errors > 0:
            fixed_parts.append(f"{total_fixed_errors} error{'s' if total_fixed_errors != 1 else ''}")
        if total_fixed_warnings > 0:
            fixed_parts.append(f"{total_fixed_warnings} warning{'s' if total_fixed_warnings != 1 else ''}")
        summary_parts.append(f"Fixed { ' and '.join(fixed_parts) }.")

    remaining_parts = []
    if total_errors > 0:
        remaining_parts.append(f"{total_errors} error{'s' if total_errors != 1 else ''}")
    if total_warnings > 0:
        remaining_parts.append(f"{total_warnings} warning{'s' if total_warnings != 1 else ''}")

    if not remaining_parts:
        if summary_parts: # If fixes were made
             summary_parts.append("No further issues found.")
        else:
             summary_parts.append("No issues found.")
    else:
        prefix = "Found" if not summary_parts else "Remaining issues:"
        summary_parts.append(f"{prefix} { ' and '.join(remaining_parts) }.")
        if summary_parts and (total_fixed_errors > 0 or total_fixed_warnings > 0):
             summary_parts[-1] += " that could not be fixed" # Clarify remaining issues

    log.info("-" * 20 + " Summary " + "-" * 20)
    log.info(" ".join(summary_parts))
    log.info("-" * (49)) # Match summary line length


def main():
    """Main execution function."""
    parser = argparse.ArgumentParser(
        description="Checks text files against configured formatting rules.",
        add_help=False # Disable default help to use custom one
    )

    # --- Basic Options ---
    parser.add_argument('-h', '--help', action='store_true', help="Display detailed help message and exit.")
    parser.add_argument('--config-help', action='store_true', help="Display help for the configuration file format and exit.")

    # --- Correction Mode ---
    correction_group = parser.add_mutually_exclusive_group()
    correction_group.add_argument('-a', '--auto-correct', action='store_true', default=False,
                                  help="Attempt to automatically correct formatting issues (modifies files).")
    correction_group.add_argument('-n', '--no-correct', action='store_false', dest='auto_correct',
                                  help="Do not correct formatting issues (default).")

    # --- Config File ---
    parser.add_argument('-f', '--format-file', type=Path, default=DEFAULT_CONFIG_PATH,
                        help=f"Path to the JSON configuration file (default: {DEFAULT_CONFIG_PATH}).")

    # --- Recursion ---
    recursion_group = parser.add_mutually_exclusive_group()
    recursion_group.add_argument('-R', '--no-recursion', action='store_false', dest='recursive', default=True,
                                 help="Do not search for files recursively in specified directories.")
    recursion_group.add_argument('-r', '--recursive', action='store_true', dest='recursive',
                                 help="Search for files recursively (default).")

    # --- File Sources ---
    file_source_group = parser.add_mutually_exclusive_group(required=True)
    file_source_group.add_argument('--files', nargs='+', metavar='FILE_OR_PATTERN',
                                   help="Specify the exact list of files/directories/patterns to check. Overrides 'dataRoots' in config.")
    file_source_group.add_argument('--add-files', nargs='+', metavar='FILE_OR_PATTERN',
                                   help="Append specified files/directories/patterns to 'dataRoots' from config.")

    try:
        args = parser.parse_args()
    except SystemExit as e:
        # Catch argparse errors (like missing required args)
        # parser.print_usage() # Optionally print usage
        exit(3) # Exit code for bad arguments

    # --- Handle Help Actions ---
    if args.help:
        print_help_message()
        exit(0)
    if args.config_help:
        print_config_help_message()
        exit(0)

    # --- Load Configuration ---
    log.info(f"Loading configuration from: {args.format_file}")
    config = load_config(args.format_file)
    if config is None:
        exit(2) # Exit code for config error

    # --- Determine Files to Check ---
    file_patterns = []
    if args.files:
        file_patterns = args.files # Use only command-line specified files
    elif args.add_files:
        config_roots = config.get("dataRoots", [])
        if not isinstance(config_roots, list):
             log.error("Configuration error: 'dataRoots' must be an array (list).")
             exit(2)
        file_patterns = config_roots + args.add_files
    else:
         # Should not happen because --files or --add-files is required by argparse group
         log.error("Internal error: No file source specified.")
         exit(1)

    if not file_patterns:
        log.warning("No data roots specified in config or via command line. No files to check.")
        exit(0)

    # --- Find Files ---
    data_files_set = set()
    log.info("Searching for files...")
    for pattern in file_patterns:
        try:
            # Use Path.glob / Path.rglob for better path handling
            p = Path(pattern)
            if "*" in pattern or "?" in pattern or "[" in pattern: # Heuristic for glob pattern
                 # Determine root for globbing
                 # Find the part of the pattern without wildcards
                 fixed_part = Path(pattern.split('*', 1)[0].split('?', 1)[0].split('[', 1)[0])
                 search_dir = fixed_part if fixed_part.is_dir() else fixed_part.parent
                 glob_pattern = pattern[len(str(search_dir)):].lstrip('/\\') # Relative pattern part

                 if not search_dir.exists():
                     log.warning(f"Directory path for pattern '{pattern}' does not exist: {search_dir}")
                     continue

                 if args.recursive:
                     found = search_dir.rglob(glob_pattern)
                 else:
                     found = search_dir.glob(glob_pattern)

                 for item in found:
                      if item.is_file():
                          data_files_set.add(item.resolve()) # Store resolved absolute paths
            else: # Not a pattern, likely a specific file or directory
                 if p.is_file():
                      data_files_set.add(p.resolve())
                 elif p.is_dir():
                     if args.recursive:
                         found = p.rglob('*') # Check all files recursively
                     else:
                         found = p.glob('*') # Check only immediate files
                     for item in found:
                          if item.is_file():
                              data_files_set.add(item.resolve())
                 else:
                     log.warning(f"Path specified does not exist or is not a file/directory: {p}")

        except Exception as e:
             log.error(f"Error processing pattern/path '{pattern}': {e}")

    if not data_files_set:
         log.warning("No files found matching the specified criteria.")
         exit(0)

    data_files = sorted(list(data_files_set))
    log.info(f"Found {len(data_files)} files to check.")

    # --- Process Files ---
    total_fixed_errors = 0
    total_fixed_warnings = 0
    total_errors = 0
    total_warnings = 0
    rules = config.get("rules", {}) # Get the rules sub-dictionary

    for file_path in data_files:
        log.debug(f"Processing: {file_path}")
        try:
            result = check_file_content(file_path, args.auto_correct, rules)
            print_file_result(file_path, result)

            total_fixed_errors += len(result.fixed_errors)
            total_fixed_warnings += len(result.fixed_warnings)
            total_errors += len(result.errors)
            total_warnings += len(result.warnings)
        except Exception as e:
            log.error(f"\n--- UNEXPECTED ERROR while processing {file_path} ---")
            log.exception(e) # Log the full traceback
            log.error("--- Continuing with next file ---")
            total_errors += 1 # Count this as an error

    # --- Print Summary and Exit ---
    print_summary(total_fixed_errors, total_fixed_warnings, total_errors, total_warnings)

    if total_errors > 0:
        exit(4) # Exit code for remaining errors
    else:
        exit(0) # Success


if __name__ == '__main__':
    main()

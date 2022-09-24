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

import re
import glob

# Script that checks for common text content formatting pitfalls not covered by spellcheck or other tests.

# The number of errors found in all files
errors = 0
# The list of errors in the current file; elements are tuples of the line numbers and the error messages
error_list = []


# Checks the format of all txt files.
def check_content_style():
    files = glob.glob('data/**/*.txt', recursive=True)
    files.sort()
    for file in files:

        f = open(file, "r")
        lines = f.readlines()
        lines = [line.removesuffix('\n') for line in lines]

        check_copyright(file, lines)
        check_ascii(file, lines)

        # Making sure errors are printed in order of their line numbers
        error_list.sort(key=lambda error: error[0])
        for (line, error) in error_list:
            print(error)
        error_list.clear()


# Checks the copyright header of the files. Unlike C++ source files, these don't contain the file's name in the header.
# The tooltips.txt file is excluded from this check, as it has no copyright header.
# Parameters:
# file: the name of the file
# lines: the lines of the file
def check_copyright(file, lines):
    if file == "data/tooltips.txt":
        return
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
        print_error(file, 1, "incorrect copyright header")
    for line in lines:
        if re.search(second, line):
            text_index += 1
        else:
            break
    else:
        if len(lines) - text_index < len(third):
            print_error(file, len(third) - 1, "incomplete copyright header")
        else:
            for index, (line, standard) in enumerate(zip(lines[text_index:], third)):
                standard = standard.strip()
                if standard != line:
                    print_error(file, text_index + index + 1, "incorrect copyright header")
                    break


# Checks whether the file contains any non-ASCII characters. Extended ASCII codes are not accepted.
# Parameters:
# file: the name of the file
# lines: the lines of the file
def check_ascii(file, lines):
    for index, line in enumerate(lines):
        for char in line:
            if ord(char) < 0 or ord(char) > 127:
                print_error(file, index + 1, "files should be plain ASCII")


# Generates and stores an error message. The error is later displayed in check_content_style().
def print_error(file, line, reason):
    error_list.append((line, "Formatting error in " + file + " line " + str(line) + ": " + reason))
    global errors
    errors += 1


if __name__ == '__main__':
    check_content_style()
    if errors > 0:
        text = "Found " + errors.__str__() + " formatting " + ("error" if errors == 1 else "errors")
        print(text)
        exit(1)
    else:
        print("No formatting errors found.")
    exit(0)

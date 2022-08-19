#!/usr/bin/python

import re
import glob

# Script that checks for common code formatting pitfalls not covered by clang-format or other tests.

# String version of the regexes for easy editing
stdOp = "+/*<>&%=|!:-"
include = ["[^([{\\s" + stdOp + "][" + stdOp + "]+[^" + stdOp + "]*"]
match_exclude = ["([+:-])\\1+", "^.->", "[" + stdOp + "][)\\]}]", "^[&*]", "\\.\\.\\.$", "^e[+-]"]
part_exclude = ["^#include", "<.*>", "^(public|protected|private|default):", "operator"]

# Precompiled version of the regexes
include = [re.compile(regex) for regex in include]
match_exclude = [re.compile(regex) for regex in match_exclude]
part_exclude = [re.compile(regex) for regex in part_exclude]

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
        line = line.strip()
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
                        write_error(line[i:i + 3], file, line_count)
                    # End of comment
                    is_multiline_comment = False
                    i += 1
                    start_index = i + 1
                continue
            if (not is_string) and first_two == "//":
                test(line[start_index:i], file, line_count)
                # Checking for space after comment
                if len(line) > i + 2:
                    if re.search(after_comment, line[i + 2:i + 3]):
                        write_error(line[i:i + 3], file, line_count)
                break
            elif (not is_string) and first_two == "/*":
                test(line[start_index:i], file, line_count)
                is_multiline_comment = True
                # Checking for space after comment
                if len(line) > i + 2 and line[i + 2] != ' ':
                    write_error(line[i:i + 3], file, line_count)
                continue
            # Checking for strings (both standard and raw literals)
            elif (not is_string) and char == "'":
                if is_char:
                    start_index = i + 1
                else:
                    test(line[start_index:i], file, line_count)
                is_char = not is_char
            elif is_char:
                continue
            elif char == '"':
                if line[i:i + 4] == "\"\"\"\"":
                    if is_raw_string:
                        start_index = i + 4
                    else:
                        test(line[start_index:i], file, line_count)
                    is_raw_string = not is_raw_string
                    is_string = not is_string
                else:
                    if is_string:
                        start_index = i + 1
                    else:
                        test(line[start_index:i], file, line_count)
                    is_string = not is_string
        else:
            if (not is_multiline_comment) and (not is_char) and (not is_escaped) and (not is_string):
                test(line[start_index:], file, line_count)


def test(text, file, line):
    # Skip empty
    if re.match(whitespace_only, text):
        return False
    for token in re.split(whitespaces, text):
        token = token.strip()
        if token != "":
            for inc in include:
                pos = re.search(inc, token)
                if pos is not None:
                    match = token[pos.start():pos.end()]
                    valid = True
                    for temp in match_exclude:
                        if re.search(temp, match):
                            valid = False
                            break
                    else:
                        for temp in part_exclude:
                            if re.search(temp, text):
                                valid = False
                                break
                    if valid:
                        write_error(token, file, line)
                        global errors
                        errors += 1
                break


def write_error(text, file, line):
    print("Formatting error in file ", file, " line ", line, ": ", text)


if __name__ == '__main__':
    check_code_style()
    if errors > 0:
        print("Found", errors, "formatting", "error" if errors == 1 else "errors", sep=" ")
        exit(1)
    print("No formatting errors found.")
    exit(0)

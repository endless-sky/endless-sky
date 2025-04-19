#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2023 by an anonymous author
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


# This script implements the cipher by Lia Gerty described here:
#
#    https://github.com/endless-sky/endless-sky/pull/7101
#
# Requires Python 3.6+ (for f-strings and type hints).

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
# INSTRUCTIONS
#
# Before you run this script, first you need to translate your desired
# phrase from English to Indonesian. Here's an example:
#
# English: Within truth, I see only lies.
# Indonesian: Dalam kebenaran, saya hanya melihat kebohongan.
#
# Next, send that Indonesian text into this script's stdin, and you'll see:
#
# ORIGINAL: Dalam kebenaran, saya hanya melihat kebohongan.
# REVERSED: malaD ,naranebek ayas aynah tahilem .nagnohobek
# EXILE:    famaS ,ralarehet aga' agrap kapimef .ranrupuhet
# EFRETI:   fanaT ,raparebes adam adrah kahinef .ralruhubes
#
# Clearly, you'll need to manually correct a few things. The commas
# and periods are in the wrong place, the wrong letters are
# capitalized, and it's probably best to avoid common names and
# religious concepts. (Adam = the first man in some religions.) You
# should also make sure the end product is pronounceable, with
# particular attention to ' characters in Exile words. These are meant
# to represent glottal stops; if you are unfamiliar, imagine what the
# word would sound like with a k in place of the '. If it is too
# difficult, add some vowels or move letters around. After manual
# tweaking, you might end up with something like this:
#
# EXILE:  Famas ralarehet, aga' agrap kapimef ranrupuhet.
# EFRETI: Fanat raparebes, adas adrah kahinef ralruhubes.
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
# GENERAL TRANSLATION NOTES
#
# There are a few words with standard translations you'll need to apply manually:
#
# humans => humanika               (Exile, Efreti, and Quarg)
# human (noun, member of species) => humani   (Exile, Efreti, and Quarg)
# Quarg => Kuwaru                  (Efreti)
# Drak => Drak                     (both Exile and Efreti)
# Ember Space => Nraol Alaj        (Exile)
# There Might Be Riots => Ter Mite Bee Riot   (both Exile and Efreti)
#
# The easiest way to do this is to put them in ALL CAPS to whatever
# translation program you're using to get Indonesian from English. The
# cipher will produce gibberish in AOFEM MCAV (that's "all caps" in
# Efreti) which you can then replace with the correct translation.
#
# A word of warning: sometimes the cipher will produce obscene or
# objectionable words, or words that are sacred to some people. Use
# discretion and manually correct them. "Ember Space" had that problem.
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

import sys
from typing import Dict, List, Tuple

# --- Cipher Mapping Definitions ---
# Based on the table from PR #7101.
# Format: (Indonesian_Char, Exile_Char, Efreti_Char)

_RAW_MAPPING_DATA: List[Tuple[str, str, str]] = [
    # Uppercase
    ('A', 'A', 'A'), ('E', 'E', 'E'), ('I', 'I', 'I'), ('O', 'U', 'U'), ('U', 'O', 'O'),
    ('B', 'H', 'B'), ('C', 'D', 'V'), ('D', 'S', 'T'), ('F', 'J', 'Y'), ('G', 'N', 'L'),
    ('H', 'P', 'H'), ('J', 'V', 'W'), ('K', 'T', 'S'), ('L', 'M', 'N'), ('M', 'F', 'F'),
    ('N', 'R', 'R'), ('P', 'B', 'C'), ('Q', 'Z', 'T'), ('R', 'L', 'P'), ('S', "'", 'M'),
    ('T', 'K', 'K'), ('V', 'Q', 'R'), ('W', 'C', 'G'), ('X', 'Y', 'S'), ('Y', 'G', 'D'),
    ('Z', 'W', 'K'),
    # Lowercase
    ('a', 'a', 'a'), ('e', 'e', 'e'), ('i', 'i', 'i'), ('o', 'u', 'u'), ('u', 'o', 'o'),
    ('b', 'h', 'b'), ('c', 'd', 'v'), ('d', 's', 't'), ('f', 'j', 'y'), ('g', 'n', 'l'),
    ('h', 'p', 'h'), ('j', 'v', 'w'), ('k', 't', 's'), ('l', 'm', 'n'), ('m', 'f', 'f'),
    ('n', 'r', 'r'), ('p', 'b', 'c'), ('q', 'z', 't'), ('r', 'l', 'p'), ('s', "'", 'm'),
    ('t', 'k', 'k'), ('v', 'q', 'r'), ('w', 'c', 'g'), ('x', 'y', 's'), ('y', 'g', 'd'),
    ('z', 'w', 'k'),
]

# --- Function to create cipher dictionaries ---

def create_cipher_maps(mapping_data: List[Tuple[str, str, str]]) -> Tuple[Dict[str, str], Dict[str, str]]:
    """Creates the Exile and Efreti cipher mapping dictionaries."""
    to_exile_map: Dict[str, str] = {}
    to_efreti_map: Dict[str, str] = {}
    for indo_char, exile_char, efreti_char in mapping_data:
        to_exile_map[indo_char] = exile_char
        to_efreti_map[indo_char] = efreti_char
    return to_exile_map, to_efreti_map

# --- Core Cipher Function ---

def apply_cipher(text: str, cipher_map: Dict[str, str]) -> str:
    """Applies the given cipher map to the text."""
    result = []
    for char in text:
        result.append(cipher_map.get(char, char)) # Use mapped char or original if not found
    return ''.join(result)

# --- Line Processing Function ---

def process_line(line: str, exile_map: Dict[str, str], efreti_map: Dict[str, str]) -> Tuple[str, str, str, str]:
    """
    Processes a single line of Indonesian text.

    Returns:
        A tuple containing: (original_joined, reversed_joined, exile_joined, efreti_joined)
    """
    original_words: List[str] = line.split() # Split line into words
    reversed_words: List[str] = []
    exile_words: List[str] = []
    efreti_words: List[str] = []

    for word in original_words:
        # 1. Reverse the word
        reversed_word = ''.join(reversed(word))
        reversed_words.append(reversed_word)

        # 2. Apply ciphers to the reversed word
        exile_word = apply_cipher(reversed_word, exile_map)
        efreti_word = apply_cipher(reversed_word, efreti_map)

        exile_words.append(exile_word)
        efreti_words.append(efreti_word)

    # Join words back into strings
    original_joined = " ".join(original_words)
    reversed_joined = " ".join(reversed_words)
    exile_joined = " ".join(exile_words)
    efreti_joined = " ".join(efreti_words)

    return original_joined, reversed_joined, exile_joined, efreti_joined

# --- Main Execution Logic ---

def main():
    """Reads stdin, processes lines, and prints results."""
    # Create the cipher maps once
    to_exile, to_efreti = create_cipher_maps(_RAW_MAPPING_DATA)

    # Process each line from standard input
    for line in sys.stdin:
        # Strip trailing newline/whitespace from input line
        cleaned_line = line.rstrip()
        if not cleaned_line: # Skip empty lines if any
            continue

        original, reversed_str, exile_str, efreti_str = process_line(cleaned_line, to_exile, to_efreti)

        # Print the results for this line using f-strings for formatted output
        print(f"ORIGINAL: {original}")
        print(f"REVERSED: {reversed_str}")
        print(f"EXILE:    {exile_str}") # Used standard spaces for alignment
        print(f"EFRETI:   {efreti_str}") # Used standard spaces for alignment
        print() # Print empty line for separation

if __name__ == "__main__":
    main()

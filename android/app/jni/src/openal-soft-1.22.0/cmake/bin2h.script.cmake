# Read the input file into 'indata', converting each byte to a pair of hex
# characters
file(READ "${INPUT_FILE}" indata HEX)

# For each pair of characters, indent them and prepend the 0x prefix, and
# append a comma separateor.
# TODO: Prettify this. Should group a number of bytes per line instead of one
# per line.
string(REGEX REPLACE "(..)" "    0x\\1,\n" output "${indata}")

# Write the list of hex chars to the output file in a const byte array
file(WRITE "${OUTPUT_FILE}"
    "const unsigned char ${VARIABLE_NAME}[] = {\n${output}};\n")

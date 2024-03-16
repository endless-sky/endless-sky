# Generate a version string

# Extract the version from the credits.txt
set(version "")
file(STRINGS "../credits.txt" matched)
if(matched)
	string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" version ${matched})
endif()

# Append the git tag
execute_process(COMMAND git describe --tags
						OUTPUT_VARIABLE mobile_version
						OUTPUT_STRIP_TRAILING_WHITESPACE)

set(ENDLESS_SKY_VERSION "${version}-${mobile_version}")

# Overwrite the existing file if the contents are different.
if(OUTPUT_FILE)
	if(EXISTS "${OUTPUT_FILE}")
		file(STRINGS "${OUTPUT_FILE}" old_version)
		if(NOT "${old_version}" STREQUAL "#define ENDLESS_SKY_VERSION \"${ENDLESS_SKY_VERSION}\"")
			file (WRITE ${OUTPUT_FILE} "#define ENDLESS_SKY_VERSION \"${ENDLESS_SKY_VERSION}\"")
		endif()
	else()
		file (WRITE ${OUTPUT_FILE} "#define ENDLESS_SKY_VERSION \"${ENDLESS_SKY_VERSION}\"")
	endif()
else()
	message("${ENDLESS_SKY_VERSION}")
endif()

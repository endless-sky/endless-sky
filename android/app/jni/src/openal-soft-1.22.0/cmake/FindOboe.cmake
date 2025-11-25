# - Find Oboe
# Find the Oboe library
#
# This module defines the following variable:
#   OBOE_FOUND - True if Oboe was found
#
# This module defines the following target:
#   oboe::oboe - Import target for linking Oboe to a project
#

find_path(OBOE_INCLUDE_DIR NAMES oboe/Oboe.h
    DOC "The Oboe include directory"
)

find_library(OBOE_LIBRARY NAMES oboe
    DOC "The Oboe library"
)

# handle the QUIETLY and REQUIRED arguments and set OBOE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Oboe REQUIRED_VARS OBOE_LIBRARY OBOE_INCLUDE_DIR)

if(OBOE_FOUND)
    add_library(oboe::oboe UNKNOWN IMPORTED)
    set_target_properties(oboe::oboe PROPERTIES
        IMPORTED_LOCATION ${OBOE_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OBOE_INCLUDE_DIR})
endif()

mark_as_advanced(OBOE_INCLUDE_DIR OBOE_LIBRARY)

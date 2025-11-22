# - Find OSS includes
#
#   OSS_FOUND        - True if OSS_INCLUDE_DIR is found
#   OSS_INCLUDE_DIRS - Set when OSS_INCLUDE_DIR is found
#   OSS_LIBRARIES    - Set when OSS_LIBRARY is found
#
#   OSS_INCLUDE_DIR - where to find sys/soundcard.h, etc.
#   OSS_LIBRARY     - where to find libossaudio (optional).
#

find_path(OSS_INCLUDE_DIR
          NAMES sys/soundcard.h
          DOC "The OSS include directory"
)

find_library(OSS_LIBRARY
             NAMES ossaudio
             DOC "Optional OSS library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSS  REQUIRED_VARS OSS_INCLUDE_DIR)

if(OSS_FOUND)
    set(OSS_INCLUDE_DIRS ${OSS_INCLUDE_DIR})
    if(OSS_LIBRARY)
        set(OSS_LIBRARIES ${OSS_LIBRARY})
    else()
        unset(OSS_LIBRARIES)
    endif()
endif()

mark_as_advanced(OSS_INCLUDE_DIR OSS_LIBRARY)

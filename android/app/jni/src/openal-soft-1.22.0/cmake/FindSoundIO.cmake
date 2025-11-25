# - Find SoundIO (sndio) includes and libraries
#
#   SOUNDIO_FOUND        - True if SOUNDIO_INCLUDE_DIR & SOUNDIO_LIBRARY are
#                          found
#   SOUNDIO_LIBRARIES    - Set when SOUNDIO_LIBRARY is found
#   SOUNDIO_INCLUDE_DIRS - Set when SOUNDIO_INCLUDE_DIR is found
#
#   SOUNDIO_INCLUDE_DIR - where to find sndio.h, etc.
#   SOUNDIO_LIBRARY     - the sndio library
#

find_path(SOUNDIO_INCLUDE_DIR
          NAMES sndio.h
          DOC "The SoundIO include directory"
)

find_library(SOUNDIO_LIBRARY
             NAMES sndio
             DOC "The SoundIO library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SoundIO
    REQUIRED_VARS SOUNDIO_LIBRARY SOUNDIO_INCLUDE_DIR
)

if(SOUNDIO_FOUND)
    set(SOUNDIO_LIBRARIES ${SOUNDIO_LIBRARY})
    set(SOUNDIO_INCLUDE_DIRS ${SOUNDIO_INCLUDE_DIR})
endif()

mark_as_advanced(SOUNDIO_INCLUDE_DIR SOUNDIO_LIBRARY)

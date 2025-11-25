# - Try to find SndFile
# Once done this will define
#
#  SNDFILE_FOUND - system has SndFile
#  SndFile::SndFile - the SndFile target
#

find_path(SNDFILE_INCLUDE_DIR NAMES sndfile.h)

find_library(SNDFILE_LIBRARY NAMES sndfile sndfile-1)

# handle the QUIETLY and REQUIRED arguments and set SNDFILE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SndFile DEFAULT_MSG SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR)

if(SNDFILE_FOUND)
    add_library(SndFile::SndFile UNKNOWN IMPORTED)
    set_target_properties(SndFile::SndFile PROPERTIES
        IMPORTED_LOCATION ${SNDFILE_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${SNDFILE_INCLUDE_DIR})
endif()

# show the SNDFILE_INCLUDE_DIR and SNDFILE_LIBRARY variables only in the advanced view
mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)

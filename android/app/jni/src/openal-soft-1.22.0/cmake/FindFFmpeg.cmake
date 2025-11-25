# vim: ts=2 sw=2
# - Try to find the required ffmpeg components(default: AVFORMAT, AVUTIL, AVCODEC)
#
# Once done this will define
#  FFMPEG_FOUND         - System has the all required components.
#  FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
#  FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
#
# For each of the components it will additionaly set.
#   - AVCODEC
#   - AVDEVICE
#   - AVFORMAT
#   - AVUTIL
#   - POSTPROC
#   - SWSCALE
#   - SWRESAMPLE
# the following variables will be defined
#  <component>_FOUND        - System has <component>
#  <component>_INCLUDE_DIRS - Include directory necessary for using the <component> headers
#  <component>_LIBRARIES    - Link these to use <component>
#  <component>_DEFINITIONS  - Compiler switches required for using <component>
#  <component>_VERSION      - The components version
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
#
# Redistribution and use is allowed according to the terms of the BSD license.

include(FindPackageHandleStandardArgs)

if(NOT FFmpeg_FIND_COMPONENTS)
    set(FFmpeg_FIND_COMPONENTS AVFORMAT AVCODEC AVUTIL)
endif()

#
### Macro: set_component_found
#
# Marks the given component as found if both *_LIBRARIES AND *_INCLUDE_DIRS is present.
#
macro(set_component_found _component)
    if(${_component}_LIBRARIES AND ${_component}_INCLUDE_DIRS)
        # message(STATUS "  - ${_component} found.")
        set(${_component}_FOUND TRUE)
    else()
        # message(STATUS "  - ${_component} not found.")
    endif()
endmacro()

#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig and then looking up the libraries and
# include directories.
#
macro(find_component _component _pkgconfig _library _header)
    if(NOT WIN32)
        # use pkg-config to get the directories and then use these values
        # in the FIND_PATH() and FIND_LIBRARY() calls
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PC_${_component} ${_pkgconfig})
        endif()
    endif()

    find_path(${_component}_INCLUDE_DIRS ${_header}
        HINTS
            ${FFMPEGSDK_INC}
            ${PC_LIB${_component}_INCLUDEDIR}
            ${PC_LIB${_component}_INCLUDE_DIRS}
        PATH_SUFFIXES
            ffmpeg
    )

    find_library(${_component}_LIBRARIES NAMES ${_library}
        HINTS
            ${FFMPEGSDK_LIB}
            ${PC_LIB${_component}_LIBDIR}
            ${PC_LIB${_component}_LIBRARY_DIRS}
    )

    STRING(REGEX REPLACE "/.*" "/version.h" _ver_header ${_header})
    if(EXISTS "${${_component}_INCLUDE_DIRS}/${_ver_header}")
        file(STRINGS "${${_component}_INCLUDE_DIRS}/${_ver_header}" version_str REGEX "^#define[\t ]+LIB${_component}_VERSION_M.*")

        string(REGEX REPLACE "^.*LIB${_component}_VERSION_MAJOR[\t ]+([0-9]*).*$" "\\1" version_maj "${version_str}")
        string(REGEX REPLACE "^.*LIB${_component}_VERSION_MINOR[\t ]+([0-9]*).*$" "\\1" version_min "${version_str}")
        string(REGEX REPLACE "^.*LIB${_component}_VERSION_MICRO[\t ]+([0-9]*).*$" "\\1" version_mic "${version_str}")
        unset(version_str)

        set(${_component}_VERSION "${version_maj}.${version_min}.${version_mic}" CACHE STRING "The ${_component} version number.")
        unset(version_maj)
        unset(version_min)
        unset(version_mic)
    endif(EXISTS "${${_component}_INCLUDE_DIRS}/${_ver_header}")
    set(${_component}_VERSION     ${PC_${_component}_VERSION}      CACHE STRING "The ${_component} version number.")
    set(${_component}_DEFINITIONS ${PC_${_component}_CFLAGS_OTHER} CACHE STRING "The ${_component} CFLAGS.")

    set_component_found(${_component})

    mark_as_advanced(
        ${_component}_INCLUDE_DIRS
        ${_component}_LIBRARIES
        ${_component}_DEFINITIONS
        ${_component}_VERSION)
endmacro()


set(FFMPEGSDK $ENV{FFMPEG_HOME})
if(FFMPEGSDK)
    set(FFMPEGSDK_INC "${FFMPEGSDK}/include")
    set(FFMPEGSDK_LIB "${FFMPEGSDK}/lib")
endif()

# Check for all possible components.
find_component(AVCODEC    libavcodec    avcodec    libavcodec/avcodec.h)
find_component(AVFORMAT   libavformat   avformat   libavformat/avformat.h)
find_component(AVDEVICE   libavdevice   avdevice   libavdevice/avdevice.h)
find_component(AVUTIL     libavutil     avutil     libavutil/avutil.h)
find_component(SWSCALE    libswscale    swscale    libswscale/swscale.h)
find_component(SWRESAMPLE libswresample swresample libswresample/swresample.h)
find_component(POSTPROC   libpostproc   postproc   libpostproc/postprocess.h)

# Check if the required components were found and add their stuff to the FFMPEG_* vars.
foreach(_component ${FFmpeg_FIND_COMPONENTS})
    if(${_component}_FOUND)
        # message(STATUS "Required component ${_component} present.")
        set(FFMPEG_LIBRARIES   ${FFMPEG_LIBRARIES}   ${${_component}_LIBRARIES})
        set(FFMPEG_DEFINITIONS ${FFMPEG_DEFINITIONS} ${${_component}_DEFINITIONS})
        list(APPEND FFMPEG_INCLUDE_DIRS ${${_component}_INCLUDE_DIRS})
    else()
        # message(STATUS "Required component ${_component} missing.")
    endif()
endforeach()

# Add libz if it exists (needed for static ffmpeg builds)
find_library(_FFmpeg_HAVE_LIBZ NAMES z)
if(_FFmpeg_HAVE_LIBZ)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${_FFmpeg_HAVE_LIBZ})
endif()

# Build the include path and library list with duplicates removed.
if(FFMPEG_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
endif()

if(FFMPEG_LIBRARIES)
    list(REMOVE_DUPLICATES FFMPEG_LIBRARIES)
endif()

# cache the vars.
set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS} CACHE STRING "The FFmpeg include directories." FORCE)
set(FFMPEG_LIBRARIES    ${FFMPEG_LIBRARIES}    CACHE STRING "The FFmpeg libraries." FORCE)
set(FFMPEG_DEFINITIONS  ${FFMPEG_DEFINITIONS}  CACHE STRING "The FFmpeg cflags." FORCE)

mark_as_advanced(FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES FFMPEG_DEFINITIONS)

# Now set the noncached _FOUND vars for the components.
foreach(_component AVCODEC AVDEVICE AVFORMAT AVUTIL POSTPROCESS SWRESAMPLE SWSCALE)
    set_component_found(${_component})
endforeach ()

# Compile the list of required vars
set(_FFmpeg_REQUIRED_VARS FFMPEG_LIBRARIES FFMPEG_INCLUDE_DIRS)
foreach(_component ${FFmpeg_FIND_COMPONENTS})
    list(APPEND _FFmpeg_REQUIRED_VARS ${_component}_LIBRARIES ${_component}_INCLUDE_DIRS)
endforeach()

# Give a nice error message if some of the required vars are missing.
find_package_handle_standard_args(FFmpeg DEFAULT_MSG ${_FFmpeg_REQUIRED_VARS})

# - Find MySOFA
# Find the MySOFA libraries
#
#  This module defines the following variables:
#     MYSOFA_FOUND        - True if MYSOFA_INCLUDE_DIR & MYSOFA_LIBRARY are found
#     MYSOFA_INCLUDE_DIRS - where to find mysofa.h, etc.
#     MYSOFA_LIBRARIES    - the MySOFA library
#

#=============================================================================
# Copyright 2009-2011 Kitware, Inc.
# Copyright 2009-2011 Philip Lowman <philip@yhbt.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
#  * The names of Kitware, Inc., the Insight Consortium, or the names of
#    any consortium members, or of any contributors, may not be used to
#    endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_package(ZLIB)

find_path(MYSOFA_INCLUDE_DIR NAMES mysofa.h
          DOC "The MySOFA include directory"
)

find_library(MYSOFA_LIBRARY NAMES mysofa
             DOC "The MySOFA library"
)

find_library(MYSOFA_M_LIBRARY NAMES m
             DOC "The math library for MySOFA"
)

# handle the QUIETLY and REQUIRED arguments and set MYSOFA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySOFA REQUIRED_VARS MYSOFA_LIBRARY MYSOFA_INCLUDE_DIR ZLIB_FOUND)

if(MYSOFA_FOUND)
    set(MYSOFA_INCLUDE_DIRS ${MYSOFA_INCLUDE_DIR})
    set(MYSOFA_LIBRARIES ${MYSOFA_LIBRARY})
    set(MYSOFA_LIBRARIES ${MYSOFA_LIBRARIES} ZLIB::ZLIB)
    if(MYSOFA_M_LIBRARY)
        set(MYSOFA_LIBRARIES ${MYSOFA_LIBRARIES} ${MYSOFA_M_LIBRARY})
    endif()

    add_library(MySOFA::MySOFA UNKNOWN IMPORTED)
    set_property(TARGET MySOFA::MySOFA PROPERTY
        IMPORTED_LOCATION ${MYSOFA_LIBRARY})
    set_target_properties(MySOFA::MySOFA PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${MYSOFA_INCLUDE_DIRS}
        INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
    if(MYSOFA_M_LIBRARY)
        set_property(TARGET MySOFA::MySOFA APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES ${MYSOFA_M_LIBRARY})
    endif()
endif()

mark_as_advanced(MYSOFA_INCLUDE_DIR MYSOFA_LIBRARY)

# - Find OpenSL
# Find the OpenSL libraries
#
#  This module defines the following variables and targets:
#     OPENSL_FOUND     - True if OPENSL was found
#     OpenSL::OpenSLES - The OpenSLES target
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

find_path(OPENSL_INCLUDE_DIR NAMES SLES/OpenSLES.h
    DOC "The OpenSL include directory")
find_path(OPENSL_ANDROID_INCLUDE_DIR NAMES SLES/OpenSLES_Android.h
    DOC "The OpenSL Android include directory")

find_library(OPENSL_LIBRARY NAMES OpenSLES
    DOC "The OpenSL library")

# handle the QUIETLY and REQUIRED arguments and set OPENSL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSL REQUIRED_VARS OPENSL_LIBRARY OPENSL_INCLUDE_DIR
    OPENSL_ANDROID_INCLUDE_DIR)

if(OPENSL_FOUND)
    add_library(OpenSL::OpenSLES UNKNOWN IMPORTED)
    set_target_properties(OpenSL::OpenSLES PROPERTIES
        IMPORTED_LOCATION ${OPENSL_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OPENSL_INCLUDE_DIR}
        INTERFACE_INCLUDE_DIRECTORIES ${OPENSL_ANDROID_INCLUDE_DIR})
endif()

mark_as_advanced(OPENSL_INCLUDE_DIR OPENSL_ANDROID_INCLUDE_DIR OPENSL_LIBRARY)

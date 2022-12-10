# Copyright (c) 2022 by Osyotr
# https://gist.github.com/Osyotr/e2d415312a10183bc7c614013494bf21
#
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

include_guard(GLOBAL)

function(x_vcpkg_bootstrap)
    if(NOT GIT_EXECUTABLE)
        find_package(Git REQUIRED)
    endif()

    set(X_VCPKG_GIT_REPOSITORY_URL "https://github.com/Microsoft/vcpkg" CACHE STRING "Vcpkg git repository")
	set(X_VCPKG_CLONE_DIR "${CMAKE_SOURCE_DIR}/vcpkg" CACHE PATH "Vcpkg clone directory")

    if(NOT EXISTS "${X_VCPKG_CLONE_DIR}/.git")
        message(STATUS "Cloning vcpkg into ${X_VCPKG_CLONE_DIR}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" clone --quiet "${X_VCPKG_GIT_REPOSITORY_URL}" "${X_VCPKG_CLONE_DIR}"
            ERROR_VARIABLE _vcpkg_git_clone_error
        )
        if(_vcpkg_git_clone_error)
            message(FATAL_ERROR "Could not clone vcpkg repository from ${X_VCPKG_GIT_REPOSITORY_URL}\nMake sure you have access rights and the URL is valid.")
        endif()
        message(STATUS "Cloning vcpkg into ${X_VCPKG_CLONE_DIR} - done")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${X_VCPKG_CLONE_DIR}"
        OUTPUT_VARIABLE _vcpkg_current_head_sha
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(_baseline_sha "${_vcpkg_current_head_sha}")

    file(READ "${CMAKE_SOURCE_DIR}/vcpkg-configuration.json" _vcpkg_configuration_json_contents)
    string(JSON _baseline_sha ERROR_VARIABLE _get_baseline_err GET "${_vcpkg_configuration_json_contents}" "default-registry" "baseline")

    set(_should_update_vcpkg FALSE)
    if(NOT "${_vcpkg_current_head_sha}" STREQUAL "${_baseline_sha}")
        set(_should_update_vcpkg TRUE)
    endif()

    if(_should_update_vcpkg)
        message(STATUS "Fetching changes from vcpkg upstream")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" fetch --quiet --prune
            WORKING_DIRECTORY "${X_VCPKG_CLONE_DIR}"
            # No error checking here to allow offline usage
        )
        message(STATUS "Fetching changes from vcpkg upstream - done")

        message(STATUS "Switching vcpkg HEAD to ${_baseline_sha}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" reset --quiet --hard "${_baseline_sha}"
            COMMAND "${GIT_EXECUTABLE}" clean -qfd
            WORKING_DIRECTORY "${X_VCPKG_CLONE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        message(STATUS "Switching vcpkg HEAD to ${_baseline_sha} - done")

        # Remove vcpkg executable to trigger bootstrap
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            file(REMOVE "${X_VCPKG_CLONE_DIR}/vcpkg.exe")
        else()
            file(REMOVE "${X_VCPKG_CLONE_DIR}/vcpkg")
        endif()
    endif()
endfunction()

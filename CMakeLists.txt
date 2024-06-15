cmake_minimum_required(VERSION 3.16...3.29)

include(CMakeDependentOption)
if(UNIX AND NOT APPLE AND NOT ES_STEAM)
	option(ES_USE_VCPKG "Use vcpkg to get dependencies." OFF)
else()
	option(ES_USE_VCPKG "Use vcpkg to get dependencies." ON)
endif()
cmake_dependent_option(ES_GLES "Build the game with OpenGL ES" OFF UNIX OFF)
cmake_dependent_option(ES_STEAM "Build the game for the Steam Linux runtime" OFF UNIX OFF)
cmake_dependent_option(ES_USE_SYSTEM_LIBRARIES "Use system libraries instead of the vcpkg ones." ON "APPLE OR ES_STEAM" OFF)
cmake_dependent_option(ES_CREATE_BUNDLE "Create a Bundle instead of an executable. Not suitable for development purposes." OFF APPLE OFF)

# Support Debug and Release configurations.
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" CACHE STRING "" FORCE)

# Use C++17 without any compiler specific extensions.
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Use LTO for Release builds only.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
# On Linux use relative RPATH.
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# Setup vcpkg.
if(ES_USE_VCPKG)
	set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
		CACHE STRING "Vcpkg toolchain file")
	# No need to bootstrap vcpkg if the toolchain file is explictly provided.
	if(CMAKE_TOOLCHAIN_FILE STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
		include(utils/vcpkg_bootstrap.cmake)
		x_vcpkg_bootstrap()
	endif()

	set(VCPKG_BOOTSTRAP_OPTIONS "-disableMetrics")
	set(VCPKG_INSTALL_OPTIONS "--no-print-usage")
	set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
	set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
endif()

if(NOT ES_USE_SYSTEM_LIBRARIES)
	# Tell vcpkg to use the required system libraries.
	list(APPEND VCPKG_MANIFEST_FEATURES "system-libs")
endif()

# Tell vcpkg to use system libraries that are missing in the Steam runtime.
if(ES_STEAM)
	list(APPEND VCPKG_MANIFEST_FEATURES "steam-libs")
endif()

# Various helpful options for IDEs.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT EndlessSky)
set(CMAKE_VS_JUST_MY_CODE_DEBUGGING ON)

project("Endless Sky" VERSION 0.10.8
	DESCRIPTION "Space exploration, trading, and combat game."
	HOMEPAGE_URL https://endless-sky.github.io/
	LANGUAGES CXX)

# Find the needed library dependencies.
find_package(SDL2 CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
if(NOT APPLE)
	find_package(GLEW REQUIRED)
endif()

# Only very new versions of OpenAL provide a config package, so try to find it manually.
find_package(OpenAL CONFIG QUIET)
if(NOT OpenAL_FOUND)
	find_library(OPENAL openal REQUIRED)
	add_library(OpenAL UNKNOWN IMPORTED)
	add_library(OpenAL::OpenAL ALIAS OpenAL)
	set_target_properties(OpenAL PROPERTIES IMPORTED_LOCATION "${OPENAL}")
endif()

# libmad doesn't provide native cmake support.
find_path(LIBMAD_INCLUDE_DIR mad.h)
find_library(LIBMAD_LIB_DEBUG NAMES mad libmad NAMES_PER_DIR PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
find_library(LIBMAD_LIB_RELEASE NAMES mad libmad NAMES_PER_DIR PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")

# Find the MinGW runtime DLLs.
if(MINGW AND WIN32)
	# On Windows copy the MinGW runtime DLLs to the output folder as well.
	# This is to avoid the situation where a user has other MinGW runtime DLLs
	# in their PATH that are incompatible with the MinGW used to compile ES.
	get_filename_component(PARENT_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
	get_filename_component(MINGW_RUNTIME_DIR "${PARENT_DIR}" DIRECTORY)

	# MinGW doesn't have seh exceptions support for 32-bit Windows unfortunately,
	# and requires libgcc_s_dw2-1.dll instead of libgcc_s_seh-1.dll. There's no
	# perfect way to figure out which one to copy, so we simply copy both.
	# The executable will choose the correct DLL anyways.
	foreach(lib "stdc++-6" "winpthread-1" "gcc_s_seh-1" "gcc_s_dw2-1")
		file(GLOB_RECURSE FILE_PATH "${MINGW_RUNTIME_DIR}/lib${lib}.dll")
		if(FILE_PATH)
			list(APPEND MINGW_RUNTIME "${FILE_PATH}")
		endif()
	endforeach()
endif()

# Add the library target.
add_subdirectory(source)

# Create an interface library that contains common libraries for the game and unit tests.
add_library(ExternalLibraries INTERFACE)
target_link_libraries(EndlessSkyLib PUBLIC ExternalLibraries)

# The 'mingw32' lib needs to be linked first.
if(MINGW)
	target_link_libraries(ExternalLibraries INTERFACE mingw32)
endif()

# Link with the general libraries.
target_link_libraries(ExternalLibraries INTERFACE SDL2::SDL2 PNG::PNG JPEG::JPEG OpenAL::OpenAL
	"$<IF:$<CONFIG:Debug>,${LIBMAD_LIB_DEBUG},${LIBMAD_LIB_RELEASE}>")

# Link the needed OS-specific dependencies, if any.
if(WIN32)
	target_link_libraries(ExternalLibraries INTERFACE rpcrt4 Winmm)
else()
	target_link_libraries(ExternalLibraries INTERFACE pthread)
endif()

# Link with the UUID library, which is different for each OS.
if(UNIX)
	if(APPLE)
		find_library(UUID_LIB NAMES System PATHS /lib /usr/lib /usr/local/lib)
		find_path(UUID_INCLUDE uuid/uuid.h /usr/local/include /usr/include)
		target_link_libraries(ExternalLibraries INTERFACE "${UUID_LIB}")
		target_include_directories(ExternalLibraries INTERFACE "${UUID_INCLUDE}")
	elseif(NOT ES_USE_VCPKG OR ES_USE_SYSTEM_LIBRARIES)
		find_library(UUID_LIB uuid)
		target_link_libraries(ExternalLibraries INTERFACE "${UUID_LIB}")
	else()
		find_package(unofficial-libuuid CONFIG REQUIRED)
		target_link_libraries(ExternalLibraries INTERFACE unofficial::UUID::uuid)
	endif()
endif()

# Link with OpenGL or OpenGL ES.
if(ES_GLES)
	find_package(OpenGL REQUIRED OpenGL EGL)
	target_link_libraries(ExternalLibraries INTERFACE OpenGL::OpenGL OpenGL::EGL)
	target_compile_definitions(EndlessSkyLib PUBLIC ES_GLES)
else()
	# The Steam runtime can't rely on GLVND being available, so link against the legacy interface.
	if(ES_STEAM)
		set(OpenGL_GL_PREFERENCE "LEGACY")
	endif()

	find_package(OpenGL REQUIRED)
	target_link_libraries(ExternalLibraries INTERFACE OpenGL::GL)

	if(APPLE)
		# Apple deprecated OpenGL in MacOS 10.14, but we don't care.
		target_compile_definitions(EndlessSkyLib PUBLIC GL_SILENCE_DEPRECATION)
	else()
		# GLEW is only needed on Linux and Windows.
		target_link_libraries(ExternalLibraries INTERFACE GLEW::glew)
	endif()
endif()

# Setup for the testing frameworks.
include(CTest)
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

# Create game target.
if(APPLE AND ES_CREATE_BUNDLE)
	add_executable(EndlessSky MACOSX_BUNDLE source/main.cpp)

	# MacOS bundles are a bit special and need every resource specified when
	# creating the executable.
	# NOTE: This means that a reconfigure is needed if any assets are added/removed!
	foreach(folder "data" "images" "sounds")
		file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/*")
		target_sources(EndlessSky PRIVATE "${RESOURCES}")

		foreach(FILE ${RESOURCES})
			# Get the relative path from the root folder to the current file.
			file(RELATIVE_PATH NEW_FILE "${CMAKE_CURRENT_SOURCE_DIR}" "${FILE}")
			# Get the parent directory for the new location.
			get_filename_component(FILE_PATH "${NEW_FILE}" DIRECTORY)

			# Resources belong under Resources/.
			set_source_files_properties("${FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${FILE_PATH}")
		endforeach()
	endforeach()

	# Create the icns file, required for the bundle icon.
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/icons/endless-sky.icns"
		COMMAND iconutil -c icns -o icons/endless-sky.icns icons/endless-sky.iconset
		DEPENDS icons/endless-sky.iconset/icon_16x16.png icons/endless-sky.iconset/icon_16x16@2x.png
			icons/endless-sky.iconset/icon_32x32.png icons/endless-sky.iconset/icon_32x32@2x.png
			icons/endless-sky.iconset/icon_128x128.png icons/endless-sky.iconset/icon_128x128@2x.png
			icons/endless-sky.iconset/icon_256x256.png icons/endless-sky.iconset/icon_256x256@2x.png
			icons/endless-sky.iconset/icon_512x512.png icons/endless-sky.iconset/icon_512x512@2x.png
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)

	# Now do the same to standalone files.
	foreach(file "license.txt" "keys.txt" "credits.txt" "copyright" "changelog" "icons/endless-sky.icns")
		target_sources(EndlessSky PRIVATE ${file})
		set_source_files_properties(${file} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/")
	endforeach()

	# Add plist to bundle.
	set_target_properties(EndlessSky PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/resources/EndlessSky-Info.plist"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME endless-sky)
	set_target_properties(EndlessSky PROPERTIES OUTPUT_NAME "Endless Sky")
elseif(WIN32)
	add_executable(EndlessSky WIN32 source/main.cpp source/WinApp.rc)
	set_target_properties(EndlessSky PROPERTIES OUTPUT_NAME "Endless Sky")
else()
	add_executable(EndlessSky source/main.cpp)
	set_target_properties(EndlessSky PROPERTIES OUTPUT_NAME "endless-sky")
endif()

# Link with the library dependencies.
target_link_libraries(EndlessSky PRIVATE ExternalLibraries EndlessSkyLib $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>)

# Copy the MinGW runtime DLLs if necessary.
if(MINGW AND WIN32)
	foreach(FILE_PATH ${MINGW_RUNTIME})
		add_custom_command(TARGET EndlessSky POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${FILE_PATH}" "$<TARGET_FILE_DIR:EndlessSky>"
			COMMAND_EXPAND_LISTS VERBATIM)

		# Add an install rule for this DLLs, so that it is also included when installing.
		install(FILES ${FILE_PATH} DESTINATION .)
	endforeach()
endif()

# Installation configurations.
if(APPLE)
	install(TARGETS EndlessSky CONFIGURATIONS Release BUNDLE DESTINATION .)
elseif(WIN32)
	# Install the binary.
	install(TARGETS EndlessSky CONFIGURATIONS Release RUNTIME DESTINATION .)

	# The MinGW DLLs needed were already installed above, and vcpkg installs the library DLLs.

	# Install the resource files.
	install(DIRECTORY data DESTINATION .)
	install(DIRECTORY images DESTINATION .)
	install(DIRECTORY sounds DESTINATION .)
	install(FILES credits.txt DESTINATION .)
	install(FILES keys.txt DESTINATION .)
	install(FILES copyright DESTINATION .)
	install(FILES changelog DESTINATION .)
	install(FILES license.txt DESTINATION .)
elseif(UNIX)
	# Install the binary.
	install(TARGETS EndlessSky CONFIGURATIONS Release RUNTIME DESTINATION games)

	# Install the desktop file.
	install(FILES io.github.endless_sky.endless_sky.desktop DESTINATION share/applications)

	# Install app center metadata.
	install(FILES io.github.endless_sky.endless_sky.appdata.xml DESTINATION share/metainfo)

	# Install icons, keeping track of all the paths.
	# Most Ubuntu apps supply 16, 22, 24, 32, 48, and 256, and sometimes others.
	foreach(size "16x16" "22x22" "24x24" "32x32" "48x48" "128x128" "256x256" "512x512")
		install(FILES "icons/icon_${size}.png" DESTINATION "share/icons/hicolor/${size}/apps"
			RENAME endless-sky.png)
	endforeach()

	# Gzip and install man file.
	add_custom_command(
		OUTPUT endless-sky.6.gz
		COMMAND gzip -c endless-sky.6 > ${CMAKE_CURRENT_BINARY_DIR}/endless-sky.6.gz
		DEPENDS endless-sky.6
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM)
	target_sources(EndlessSky PRIVATE endless-sky.6.gz)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/endless-sky.6.gz DESTINATION share/man/man6)

	# Install the resource files.
	install(DIRECTORY data DESTINATION share/games/endless-sky)
	install(DIRECTORY images DESTINATION share/games/endless-sky)
	install(DIRECTORY sounds DESTINATION share/games/endless-sky)
	install(FILES credits.txt DESTINATION share/games/endless-sky)
	install(FILES keys.txt DESTINATION share/games/endless-sky)
	install(FILES copyright DESTINATION share/doc/endless-sky)
	install(FILES changelog DESTINATION share/doc/endless-sky)
	install(FILES license.txt DESTINATION share/doc/endless-sky)
endif()

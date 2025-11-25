# CMake build file list for OpenAL

cmake_minimum_required(VERSION 3.0.2)

if(APPLE)
    # The workaround for try_compile failing with code signing
    # since cmake-3.18.2, not required
    set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
        ${CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}
        "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED"
        "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # Fix compile failure with armv7 deployment target >= 11.0, xcode clang
    # will report:
    # error: invalid iOS deployment version '--target=armv7-apple-ios13.6',
    # iOS 10 is the maximum deployment target for 32-bit targets
    # If CMAKE_OSX_DEPLOYMENT_TARGET is not defined, cmake will choose latest
    # deployment target
    if("${CMAKE_OSX_ARCHITECTURES}" MATCHES ".*armv7.*")
        if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET
            OR NOT CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "11.0")
            message(STATUS "Forcing iOS deployment target to 10.0 for armv7")
            set(CMAKE_OSX_DEPLOYMENT_TARGET "10.0" CACHE STRING "Minimum OS X deployment version"
                FORCE)
        endif()
    endif()
endif()

project(OpenAL)

if(COMMAND CMAKE_POLICY)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif(POLICY CMP0020)
    if(POLICY CMP0042)
        cmake_policy(SET CMP0042 NEW)
    endif(POLICY CMP0042)
    if(POLICY CMP0054)
        cmake_policy(SET CMP0054 NEW)
    endif(POLICY CMP0054)
    if(POLICY CMP0075)
        cmake_policy(SET CMP0075 NEW)
    endif(POLICY CMP0075)
endif(COMMAND CMAKE_POLICY)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()
if(NOT CMAKE_DEBUG_POSTFIX)
    set(CMAKE_DEBUG_POSTFIX "" CACHE STRING
        "Library postfix for debug builds. Normally left blank."
        FORCE)
endif()

set(CMAKE_MODULE_PATH "${OpenAL_SOURCE_DIR}/cmake")

include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckStructHasMember)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(PkgConfig)


option(ALSOFT_DLOPEN  "Check for the dlopen API for loading optional libs"  ON)

option(ALSOFT_WERROR  "Treat compile warnings as errors"      OFF)

option(ALSOFT_UTILS "Build utility programs"  ON)
option(ALSOFT_NO_CONFIG_UTIL "Disable building the alsoft-config utility" OFF)

option(ALSOFT_EXAMPLES  "Build example programs"  ON)

option(ALSOFT_INSTALL "Install main library" ON)
option(ALSOFT_INSTALL_CONFIG "Install alsoft.conf sample configuration file" ON)
option(ALSOFT_INSTALL_HRTF_DATA "Install HRTF data files" ON)
option(ALSOFT_INSTALL_AMBDEC_PRESETS "Install AmbDec preset files" ON)
option(ALSOFT_INSTALL_EXAMPLES "Install example programs (alplay, alstream, ...)" ON)
option(ALSOFT_INSTALL_UTILS "Install utility programs (openal-info, alsoft-config, ...)" ON)
option(ALSOFT_UPDATE_BUILD_VERSION "Update git build version info" ON)

option(ALSOFT_EAX "Enable legacy EAX extensions" ${WIN32})

if(DEFINED SHARE_INSTALL_DIR)
    message(WARNING "SHARE_INSTALL_DIR is deprecated.  Use the variables provided by the GNUInstallDirs module instead")
    set(CMAKE_INSTALL_DATADIR "${SHARE_INSTALL_DIR}")
endif()

if(DEFINED LIB_SUFFIX)
    message(WARNING "LIB_SUFFIX is deprecated.  Use the variables provided by the GNUInstallDirs module instead")
endif()
if(DEFINED ALSOFT_CONFIG)
    message(WARNING "ALSOFT_CONFIG is deprecated. Use ALSOFT_INSTALL_CONFIG instead")
endif()
if(DEFINED ALSOFT_HRTF_DEFS)
    message(WARNING "ALSOFT_HRTF_DEFS is deprecated. Use ALSOFT_INSTALL_HRTF_DATA instead")
endif()
if(DEFINED ALSOFT_AMBDEC_PRESETS)
    message(WARNING "ALSOFT_AMBDEC_PRESETS is deprecated. Use ALSOFT_INSTALL_AMBDEC_PRESETS instead")
endif()


set(CPP_DEFS ) # C pre-processor, not C++
set(INC_PATHS )
set(C_FLAGS )
set(LINKER_FLAGS )
set(EXTRA_LIBS )

if(WIN32)
    set(CPP_DEFS ${CPP_DEFS} _WIN32 NOMINMAX)
    if(MINGW)
        set(CPP_DEFS ${CPP_DEFS} __USE_MINGW_ANSI_STDIO)
    endif()

    option(ALSOFT_BUILD_ROUTER  "Build the router (EXPERIMENTAL; creates OpenAL32.dll and soft_oal.dll)"  OFF)
    if(MINGW)
        option(ALSOFT_BUILD_IMPORT_LIB "Build an import .lib using dlltool (requires sed)" ON)
    endif()
elseif(APPLE)
    option(ALSOFT_OSX_FRAMEWORK "Build as macOS framework" OFF)
endif()


# QNX's gcc do not uses /usr/include and /usr/lib pathes by default
if("${CMAKE_C_PLATFORM_ID}" STREQUAL "QNX")
    set(INC_PATHS ${INC_PATHS} /usr/include)
    set(LINKER_FLAGS ${LINKER_FLAGS} -L/usr/lib)
endif()

# When the library is built for static linking, apps should define
# AL_LIBTYPE_STATIC when including the AL headers.
if(NOT LIBTYPE)
    set(LIBTYPE SHARED)
endif()

set(LIB_MAJOR_VERSION "1")
set(LIB_MINOR_VERSION "22")
set(LIB_REVISION "0")
set(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_REVISION}")
set(LIB_VERSION_NUM ${LIB_MAJOR_VERSION},${LIB_MINOR_VERSION},${LIB_REVISION},0)

set(EXPORT_DECL "")


# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Prefer C11, but support C99 and C90 too.
set(CMAKE_C_STANDARD 11)

if(NOT WIN32)
    # Check if _POSIX_C_SOURCE and _XOPEN_SOURCE needs to be set for POSIX functions
    check_symbol_exists(posix_memalign stdlib.h HAVE_POSIX_MEMALIGN_DEFAULT)
    if(NOT HAVE_POSIX_MEMALIGN_DEFAULT)
        set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600")
        check_symbol_exists(posix_memalign stdlib.h HAVE_POSIX_MEMALIGN_POSIX)
        if(NOT HAVE_POSIX_MEMALIGN_POSIX)
            set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
        else()
            set(CPP_DEFS ${CPP_DEFS} _POSIX_C_SOURCE=200112L _XOPEN_SOURCE=600)
        endif()
    endif()
    unset(OLD_REQUIRED_FLAGS)
endif()

# C99 has restrict, but C++ does not, so we can only utilize __restrict.
check_cxx_source_compiles("int *__restrict foo;
int main() { return 0; }" HAVE___RESTRICT)
if(HAVE___RESTRICT)
    set(CPP_DEFS ${CPP_DEFS} RESTRICT=__restrict)
else()
    set(CPP_DEFS ${CPP_DEFS} "RESTRICT=")
endif()

# Some systems may need libatomic for atomic functions to work
set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES} atomic)
check_cxx_source_compiles("#include <atomic>
std::atomic<int> foo{0};
int main() { return foo.fetch_add(2); }"
HAVE_LIBATOMIC)
if(NOT HAVE_LIBATOMIC)
    set(CMAKE_REQUIRED_LIBRARIES "${OLD_REQUIRED_LIBRARIES}")
else()
    set(EXTRA_LIBS atomic ${EXTRA_LIBS})
endif()
unset(OLD_REQUIRED_LIBRARIES)

if(ANDROID)
    # Include liblog for Android logging
    check_library_exists(log __android_log_print "" HAVE_LIBLOG)
    if(HAVE_LIBLOG)
        set(EXTRA_LIBS log ${EXTRA_LIBS})
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} log)
    endif()
endif()

if(MSVC)
    set(CPP_DEFS ${CPP_DEFS} _CRT_SECURE_NO_WARNINGS)
    check_cxx_compiler_flag(/permissive- HAVE_PERMISSIVE_SWITCH)
    if(HAVE_PERMISSIVE_SWITCH)
        set(C_FLAGS ${C_FLAGS} $<$<COMPILE_LANGUAGE:CXX>:/permissive->)
    endif()
    set(C_FLAGS ${C_FLAGS} /W4 /w14640 /wd4065 /wd4127 /wd4268 /wd4324 /wd5030)
    # Remove /W3, which is added by default, since we set /W4. Some build
    # generators with MSVC complain about both /W3 and /W4 being specified.
    foreach(flag_var  CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        if(${flag_var} MATCHES "/W3")
            string(REGEX REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()

    if(NOT DXSDK_DIR)
        string(REGEX REPLACE "\\\\" "/" DXSDK_DIR "$ENV{DXSDK_DIR}")
    else()
        string(REGEX REPLACE "\\\\" "/" DXSDK_DIR "${DXSDK_DIR}")
    endif()
    if(DXSDK_DIR)
        message(STATUS "Using DirectX SDK directory: ${DXSDK_DIR}")
    endif()

    option(FORCE_STATIC_VCRT "Force /MT for static VC runtimes" OFF)
    if(FORCE_STATIC_VCRT)
        foreach(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach(flag_var)
    endif()
else()
    set(C_FLAGS ${C_FLAGS} -Winline -Wunused -Wall -Wextra -Wshadow -Wconversion -Wcast-align
        -Wpedantic
        $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual>)

    if(ALSOFT_WERROR)
        set(C_FLAGS ${C_FLAGS} -Werror)
    endif()

    # We want RelWithDebInfo to actually include debug stuff (define _DEBUG
    # instead of NDEBUG)
    foreach(flag_var  CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "-DNDEBUG")
            string(REGEX REPLACE "-DNDEBUG" "-D_DEBUG" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()

    check_c_compiler_flag(-fno-math-errno HAVE_FNO_MATH_ERRNO)
    if(HAVE_FNO_MATH_ERRNO)
        set(C_FLAGS ${C_FLAGS} -fno-math-errno)
    endif()

    option(ALSOFT_STATIC_LIBGCC "Force -static-libgcc for static GCC runtimes" OFF)
    if(ALSOFT_STATIC_LIBGCC)
        set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} -static-libgcc)
        check_cxx_source_compiles("int main() { }" HAVE_STATIC_LIBGCC_SWITCH)
        set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES})
        unset(OLD_REQUIRED_LIBRARIES)

        if(NOT HAVE_STATIC_LIBGCC_SWITCH)
            message(FATAL_ERROR "Cannot static link libgcc")
        endif()
        set(LINKER_FLAGS ${LINKER_FLAGS} -static-libgcc)
    endif()

    option(ALSOFT_STATIC_STDCXX "Static link libstdc++" OFF)
    if(ALSOFT_STATIC_STDCXX)
        set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} "-Wl,--push-state,-Bstatic,-lstdc++,--pop-state")
        check_cxx_source_compiles("int main() { }" HAVE_STATIC_LIBSTDCXX_SWITCH)
        set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES})
        unset(OLD_REQUIRED_LIBRARIES)

        if(NOT HAVE_STATIC_LIBSTDCXX_SWITCH)
            message(FATAL_ERROR "Cannot static link libstdc++")
        endif()
        set(LINKER_FLAGS ${LINKER_FLAGS} "-Wl,--push-state,-Bstatic,-lstdc++,--pop-state")
    endif()

    if(WIN32)
        option(ALSOFT_STATIC_WINPTHREAD "Static link libwinpthread" OFF)
        if(ALSOFT_STATIC_WINPTHREAD)
            set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
            set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} "-Wl,--push-state,-Bstatic,-lwinpthread,--pop-state")
            check_cxx_source_compiles("int main() { }" HAVE_STATIC_LIBWINPTHREAD_SWITCH)
            set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES})
            unset(OLD_REQUIRED_LIBRARIES)

            if(NOT HAVE_STATIC_LIBWINPTHREAD_SWITCH)
                message(FATAL_ERROR "Cannot static link libwinpthread")
            endif()
            set(LINKER_FLAGS ${LINKER_FLAGS} "-Wl,--push-state,-Bstatic,-lwinpthread,--pop-state")
        endif()
    endif()
endif()

# Set visibility/export options if available
if(WIN32)
    if(NOT LIBTYPE STREQUAL "STATIC")
        set(EXPORT_DECL "__declspec(dllexport)")
    endif()
else()
    set(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    # Yes GCC, really don't accept visibility modes you don't support
    set(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -Wattributes -Werror")

    check_c_source_compiles("int foo() __attribute__((visibility(\"protected\")));
                             int main() {return 0;}" HAVE_GCC_PROTECTED_VISIBILITY)
    if(HAVE_GCC_PROTECTED_VISIBILITY)
        if(NOT LIBTYPE STREQUAL "STATIC")
            set(EXPORT_DECL "__attribute__((visibility(\"protected\")))")
        endif()
    else()
        check_c_source_compiles("int foo() __attribute__((visibility(\"default\")));
                                 int main() {return 0;}" HAVE_GCC_DEFAULT_VISIBILITY)
        if(HAVE_GCC_DEFAULT_VISIBILITY)
            if(NOT LIBTYPE STREQUAL "STATIC")
                set(EXPORT_DECL "__attribute__((visibility(\"default\")))")
            endif()
        endif()
    endif()

    if(HAVE_GCC_PROTECTED_VISIBILITY OR HAVE_GCC_DEFAULT_VISIBILITY)
        check_c_compiler_flag(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN_SWITCH)
        if(HAVE_VISIBILITY_HIDDEN_SWITCH)
            set(C_FLAGS ${C_FLAGS} -fvisibility=hidden)
        endif()
    endif()

    set(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
endif()


set(SSE2_SWITCH "")

if(NOT MSVC)
    set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    # Yes GCC, really don't accept command line options you don't support
    set(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -Werror")
    check_c_compiler_flag(-msse2 HAVE_MSSE2_SWITCH)
    if(HAVE_MSSE2_SWITCH)
        set(SSE2_SWITCH "-msse2")
    endif()
    set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
    unset(OLD_REQUIRED_FLAGS)
endif()

check_include_file(xmmintrin.h HAVE_XMMINTRIN_H)
check_include_file(emmintrin.h HAVE_EMMINTRIN_H)
check_include_file(pmmintrin.h HAVE_PMMINTRIN_H)
check_include_file(smmintrin.h HAVE_SMMINTRIN_H)
check_include_file(arm_neon.h HAVE_ARM_NEON_H)

set(HAVE_SSE        0)
set(HAVE_SSE2       0)
set(HAVE_SSE3       0)
set(HAVE_SSE4_1     0)
set(HAVE_NEON       0)

# Check for SSE support
option(ALSOFT_REQUIRE_SSE "Require SSE support" OFF)
if(HAVE_XMMINTRIN_H)
    option(ALSOFT_CPUEXT_SSE "Enable SSE support" ON)
    if(ALSOFT_CPUEXT_SSE)
        set(HAVE_SSE 1)
    endif()
endif()
if(ALSOFT_REQUIRE_SSE AND NOT HAVE_SSE)
    message(FATAL_ERROR "Failed to enabled required SSE CPU extensions")
endif()

option(ALSOFT_REQUIRE_SSE2 "Require SSE2 support" OFF)
if(HAVE_EMMINTRIN_H)
    option(ALSOFT_CPUEXT_SSE2 "Enable SSE2 support" ON)
    if(HAVE_SSE AND ALSOFT_CPUEXT_SSE2)
        set(HAVE_SSE2 1)
    endif()
endif()
if(ALSOFT_REQUIRE_SSE2 AND NOT HAVE_SSE2)
    message(FATAL_ERROR "Failed to enable required SSE2 CPU extensions")
endif()

option(ALSOFT_REQUIRE_SSE3 "Require SSE3 support" OFF)
if(HAVE_PMMINTRIN_H)
    option(ALSOFT_CPUEXT_SSE3 "Enable SSE3 support" ON)
    if(HAVE_SSE2 AND ALSOFT_CPUEXT_SSE3)
        set(HAVE_SSE3 1)
    endif()
endif()
if(ALSOFT_REQUIRE_SSE3 AND NOT HAVE_SSE3)
    message(FATAL_ERROR "Failed to enable required SSE3 CPU extensions")
endif()

option(ALSOFT_REQUIRE_SSE4_1 "Require SSE4.1 support" OFF)
if(HAVE_SMMINTRIN_H)
    option(ALSOFT_CPUEXT_SSE4_1 "Enable SSE4.1 support" ON)
    if(HAVE_SSE3 AND ALSOFT_CPUEXT_SSE4_1)
        set(HAVE_SSE4_1 1)
    endif()
endif()
if(ALSOFT_REQUIRE_SSE4_1 AND NOT HAVE_SSE4_1)
    message(FATAL_ERROR "Failed to enable required SSE4.1 CPU extensions")
endif()

# Check for ARM Neon support
option(ALSOFT_REQUIRE_NEON "Require ARM NEON support" OFF)
if(HAVE_ARM_NEON_H)
    option(ALSOFT_CPUEXT_NEON "Enable ARM NEON support" ON)
    if(ALSOFT_CPUEXT_NEON)
        check_c_source_compiles("#include <arm_neon.h>
            int main()
            {
                int32x4_t ret4 = vdupq_n_s32(0);
                return vgetq_lane_s32(ret4, 0);
            }" HAVE_NEON_INTRINSICS)
        if(HAVE_NEON_INTRINSICS)
            set(HAVE_NEON 1)
        endif()
    endif()
endif()
if(ALSOFT_REQUIRE_NEON AND NOT HAVE_NEON)
    message(FATAL_ERROR "Failed to enabled required ARM NEON CPU extensions")
endif()


set(SSE_FLAGS )
set(FPMATH_SET "0")
if(CMAKE_SIZEOF_VOID_P MATCHES "4" AND HAVE_SSE2)
    option(ALSOFT_ENABLE_SSE2_CODEGEN "Enable SSE2 code generation instead of x87 for 32-bit targets." TRUE)
    if(ALSOFT_ENABLE_SSE2_CODEGEN)
        if(MSVC)
            check_c_compiler_flag("/arch:SSE2" HAVE_ARCH_SSE2)
            if(HAVE_ARCH_SSE2)
                set(SSE_FLAGS ${SSE_FLAGS} "/arch:SSE2")
                set(C_FLAGS ${C_FLAGS} ${SSE_FLAGS})
                set(FPMATH_SET 2)
            endif()
        elseif(SSE2_SWITCH)
            check_c_compiler_flag("${SSE2_SWITCH} -mfpmath=sse" HAVE_MFPMATH_SSE_2)
            if(HAVE_MFPMATH_SSE_2)
                set(SSE_FLAGS ${SSE_FLAGS} ${SSE2_SWITCH} -mfpmath=sse)
                set(C_FLAGS ${C_FLAGS} ${SSE_FLAGS})
                set(FPMATH_SET 2)
            endif()
            # SSE depends on a 16-byte aligned stack, and by default, GCC
            # assumes the stack is suitably aligned. Older Linux code or other
            # OSs don't guarantee this on 32-bit, so externally-callable
            # functions need to ensure an aligned stack.
            set(EXPORT_DECL "${EXPORT_DECL} __attribute__((force_align_arg_pointer))")
        endif()
    endif()
endif()

if(HAVE_SSE2)
    set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    foreach(flag_var ${SSE_FLAGS})
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${flag_var}")
    endforeach()

    check_c_source_compiles("#include <emmintrin.h>
        int main() {_mm_pause(); return 0;}" HAVE_SSE_INTRINSICS)

    set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
endif()


check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(cpuid.h HAVE_CPUID_H)
check_include_file(intrin.h HAVE_INTRIN_H)
check_include_file(guiddef.h HAVE_GUIDDEF_H)
if(NOT HAVE_GUIDDEF_H)
    check_include_file(initguid.h HAVE_INITGUID_H)
endif()

# Some systems need libm for some math functions to work
set(MATH_LIB )
check_library_exists(m pow "" HAVE_LIBM)
if(HAVE_LIBM)
    set(MATH_LIB ${MATH_LIB} m)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} m)
endif()

# Some systems need to link with -lrt for clock_gettime as used by the common
# eaxmple functions.
set(RT_LIB )
check_library_exists(rt clock_gettime "" HAVE_LIBRT)
if(HAVE_LIBRT)
    set(RT_LIB rt)
endif()

# Check for the dlopen API (for dynamicly loading backend libs)
if(ALSOFT_DLOPEN)
    check_include_file(dlfcn.h HAVE_DLFCN_H)
    check_library_exists(dl dlopen "" HAVE_LIBDL)
    if(HAVE_LIBDL)
        set(EXTRA_LIBS dl ${EXTRA_LIBS})
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} dl)
    endif()
endif()

# Check for a cpuid intrinsic
if(HAVE_CPUID_H)
    check_c_source_compiles("#include <cpuid.h>
        int main()
        {
            unsigned int eax, ebx, ecx, edx;
            return __get_cpuid(0, &eax, &ebx, &ecx, &edx);
        }" HAVE_GCC_GET_CPUID)
endif()
if(HAVE_INTRIN_H)
    check_c_source_compiles("#include <intrin.h>
        int main()
        {
            int regs[4];
            __cpuid(regs, 0);
            return regs[0];
        }" HAVE_CPUID_INTRINSIC)
endif()

check_symbol_exists(posix_memalign   stdlib.h HAVE_POSIX_MEMALIGN)
check_symbol_exists(_aligned_malloc  malloc.h HAVE__ALIGNED_MALLOC)
check_symbol_exists(proc_pidpath     libproc.h HAVE_PROC_PIDPATH)

if(NOT WIN32)
    # We need pthreads outside of Windows, for semaphores. It's also used to
    # set the priority and name of threads, when possible.
    check_include_file(pthread.h HAVE_PTHREAD_H)
    if(NOT HAVE_PTHREAD_H)
        message(FATAL_ERROR "PThreads is required for non-Windows builds!")
    endif()

    check_c_compiler_flag(-pthread HAVE_PTHREAD)
    if(HAVE_PTHREAD)
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -pthread")
        set(C_FLAGS ${C_FLAGS} -pthread)
        set(LINKER_FLAGS ${LINKER_FLAGS} -pthread)
    endif()

    check_symbol_exists(pthread_setschedparam pthread.h HAVE_PTHREAD_SETSCHEDPARAM)

    # Some systems need pthread_np.h to get pthread_setname_np
    check_include_files("pthread.h;pthread_np.h" HAVE_PTHREAD_NP_H)
    if(HAVE_PTHREAD_NP_H)
        check_symbol_exists(pthread_setname_np "pthread.h;pthread_np.h" HAVE_PTHREAD_SETNAME_NP)
        if(NOT HAVE_PTHREAD_SETNAME_NP)
            check_symbol_exists(pthread_set_name_np "pthread.h;pthread_np.h" HAVE_PTHREAD_SET_NAME_NP)
        endif()
    else()
        check_symbol_exists(pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
        if(NOT HAVE_PTHREAD_SETNAME_NP)
            check_symbol_exists(pthread_set_name_np pthread.h HAVE_PTHREAD_SET_NAME_NP)
        endif()
    endif()
endif()

check_symbol_exists(getopt unistd.h HAVE_GETOPT)


# Common sources used by both the OpenAL implementation library, the OpenAL
# router, and certain tools and examples.
set(COMMON_OBJS
    common/albit.h
    common/albyte.h
    common/alcomplex.cpp
    common/alcomplex.h
    common/aldeque.h
    common/alfstream.cpp
    common/alfstream.h
    common/almalloc.cpp
    common/almalloc.h
    common/alnumbers.h
    common/alnumeric.h
    common/aloptional.h
    common/alspan.h
    common/alstring.cpp
    common/alstring.h
    common/atomic.h
    common/comptr.h
    common/dynload.cpp
    common/dynload.h
    common/intrusive_ptr.h
    common/opthelpers.h
    common/phase_shifter.h
    common/polyphase_resampler.cpp
    common/polyphase_resampler.h
    common/pragmadefs.h
    common/ringbuffer.cpp
    common/ringbuffer.h
    common/strutils.cpp
    common/strutils.h
    common/threads.cpp
    common/threads.h
    common/vecmat.h
    common/vector.h)

# Core library routines
set(CORE_OBJS
    core/ambdec.cpp
    core/ambdec.h
    core/ambidefs.cpp
    core/ambidefs.h
    core/async_event.h
    core/bformatdec.cpp
    core/bformatdec.h
    core/bs2b.cpp
    core/bs2b.h
    core/bsinc_defs.h
    core/bsinc_tables.cpp
    core/bsinc_tables.h
    core/bufferline.h
    core/buffer_storage.cpp
    core/buffer_storage.h
    core/context.cpp
    core/context.h
    core/converter.cpp
    core/converter.h
    core/cpu_caps.cpp
    core/cpu_caps.h
    core/devformat.cpp
    core/devformat.h
    core/device.cpp
    core/device.h
    core/effects/base.h
    core/effectslot.cpp
    core/effectslot.h
    core/except.cpp
    core/except.h
    core/filters/biquad.h
    core/filters/biquad.cpp
    core/filters/nfc.cpp
    core/filters/nfc.h
    core/filters/splitter.cpp
    core/filters/splitter.h
    core/fmt_traits.cpp
    core/fmt_traits.h
    core/fpu_ctrl.cpp
    core/fpu_ctrl.h
    core/front_stablizer.h
    core/helpers.cpp
    core/helpers.h
    core/hrtf.cpp
    core/hrtf.h
    core/logging.cpp
    core/logging.h
    core/mastering.cpp
    core/mastering.h
    core/mixer.cpp
    core/mixer.h
    core/resampler_limits.h
    core/uhjfilter.cpp
    core/uhjfilter.h
    core/uiddefs.cpp
    core/voice.cpp
    core/voice.h
    core/voice_change.h)

set(HAVE_RTKIT 0)
option(ALSOFT_REQUIRE_RTKIT "Require RTKit/D-Bus support" FALSE)
find_package(DBus1 QUIET)
if(DBus1_FOUND)
    option(ALSOFT_RTKIT "Enable RTKit support" ON)
    if(ALSOFT_RTKIT)
        set(HAVE_RTKIT 1)
        set(CORE_OBJS ${CORE_OBJS} core/dbus_wrap.cpp core/dbus_wrap.h core/rtkit.cpp core/rtkit.h)
        if(WIN32 OR HAVE_DLFCN_H)
            set(INC_PATHS ${INC_PATHS} ${DBus1_INCLUDE_DIRS})
            set(CPP_DEFS ${CPP_DEFS} ${DBus1_DEFINITIONS})
        else()
            set(EXTRA_LIBS ${EXTRA_LIBS} ${DBus1_LIBRARIES})
        endif()
    endif()
else()
    set(MISSING_VARS "")
    if(NOT DBus1_INCLUDE_DIRS)
        set(MISSING_VARS "${MISSING_VARS} DBus1_INCLUDE_DIRS")
    endif()
    if(NOT DBus1_LIBRARIES)
        set(MISSING_VARS "${MISSING_VARS} DBus1_LIBRARIES")
    endif()
    message(STATUS "Could NOT find DBus1 (missing:${MISSING_VARS})")
    unset(MISSING_VARS)
endif()
if(ALSOFT_REQUIRE_RTKIT AND NOT HAVE_RTKIT)
    message(FATAL_ERROR "Failed to enabled required RTKit support")
endif()

# Default mixers, always available
set(CORE_OBJS ${CORE_OBJS}
    core/mixer/defs.h
    core/mixer/hrtfbase.h
    core/mixer/hrtfdefs.h
    core/mixer/mixer_c.cpp)

# AL and related routines
set(OPENAL_OBJS
    al/auxeffectslot.cpp
    al/auxeffectslot.h
    al/buffer.cpp
    al/buffer.h
    al/effect.cpp
    al/effect.h
    al/effects/autowah.cpp
    al/effects/chorus.cpp
    al/effects/compressor.cpp
    al/effects/convolution.cpp
    al/effects/dedicated.cpp
    al/effects/distortion.cpp
    al/effects/echo.cpp
	al/effects/effects.cpp
    al/effects/effects.h
    al/effects/equalizer.cpp
    al/effects/fshifter.cpp
    al/effects/modulator.cpp
    al/effects/null.cpp
    al/effects/pshifter.cpp
    al/effects/reverb.cpp
    al/effects/vmorpher.cpp
    al/error.cpp
    al/event.cpp
    al/event.h
    al/extension.cpp
    al/filter.cpp
    al/filter.h
    al/listener.cpp
    al/listener.h
    al/source.cpp
    al/source.h
    al/state.cpp)

# ALC and related routines
set(ALC_OBJS
    alc/alc.cpp
    alc/alu.cpp
    alc/alu.h
    alc/alconfig.cpp
    alc/alconfig.h
    alc/context.cpp
    alc/context.h
    alc/device.cpp
    alc/device.h
    alc/effects/base.h
    alc/effects/autowah.cpp
    alc/effects/chorus.cpp
    alc/effects/compressor.cpp
    alc/effects/convolution.cpp
    alc/effects/dedicated.cpp
    alc/effects/distortion.cpp
    alc/effects/echo.cpp
    alc/effects/equalizer.cpp
    alc/effects/fshifter.cpp
    alc/effects/modulator.cpp
    alc/effects/null.cpp
    alc/effects/pshifter.cpp
    alc/effects/reverb.cpp
    alc/effects/vmorpher.cpp
    alc/inprogext.h
    alc/panning.cpp)

if (ALSOFT_EAX)
    set(OPENAL_OBJS
        ${OPENAL_OBJS}
        al/eax_api.cpp
        al/eax_api.h
        al/eax_eax_call.cpp
        al/eax_eax_call.h
        al/eax_effect.cpp
        al/eax_effect.h
        al/eax_exception.cpp
        al/eax_exception.h
        al/eax_fx_slot_index.cpp
        al/eax_fx_slot_index.h
        al/eax_fx_slots.cpp
        al/eax_fx_slots.h
        al/eax_globals.cpp
        al/eax_globals.h
        al/eax_utils.cpp
        al/eax_utils.h
        al/eax_x_ram.cpp
        al/eax_x_ram.h
)
endif ()

# Include SIMD mixers
set(CPU_EXTS "Default")
if(HAVE_SSE)
    set(CORE_OBJS  ${CORE_OBJS} core/mixer/mixer_sse.cpp)
    set(CPU_EXTS "${CPU_EXTS}, SSE")
endif()
if(HAVE_SSE2)
    set(CORE_OBJS  ${CORE_OBJS} core/mixer/mixer_sse2.cpp)
    set(CPU_EXTS "${CPU_EXTS}, SSE2")
endif()
if(HAVE_SSE3)
    set(CORE_OBJS  ${CORE_OBJS} core/mixer/mixer_sse3.cpp)
    set(CPU_EXTS "${CPU_EXTS}, SSE3")
endif()
if(HAVE_SSE4_1)
    set(CORE_OBJS  ${CORE_OBJS} core/mixer/mixer_sse41.cpp)
    set(CPU_EXTS "${CPU_EXTS}, SSE4.1")
endif()
if(HAVE_NEON)
    set(CORE_OBJS  ${CORE_OBJS} core/mixer/mixer_neon.cpp)
    set(CPU_EXTS "${CPU_EXTS}, Neon")
endif()


set(HAVE_ALSA       0)
set(HAVE_OSS        0)
set(HAVE_PIPEWIRE   0)
set(HAVE_SOLARIS    0)
set(HAVE_SNDIO      0)
set(HAVE_DSOUND     0)
set(HAVE_WASAPI     0)
set(HAVE_WINMM      0)
set(HAVE_PORTAUDIO  0)
set(HAVE_PULSEAUDIO 0)
set(HAVE_COREAUDIO  0)
set(HAVE_OPENSL     0)
set(HAVE_OBOE       0)
set(HAVE_WAVE       0)
set(HAVE_SDL2       0)

if(WIN32 OR HAVE_DLFCN_H)
    set(IS_LINKED "")
    macro(ADD_BACKEND_LIBS _LIBS)
    endmacro()
else()
    set(IS_LINKED " (linked)")
    macro(ADD_BACKEND_LIBS _LIBS)
        set(EXTRA_LIBS ${_LIBS} ${EXTRA_LIBS})
    endmacro()
endif()

set(BACKENDS "")
set(ALC_OBJS  ${ALC_OBJS}
    alc/backends/base.cpp
    alc/backends/base.h
    # Default backends, always available
    alc/backends/loopback.cpp
    alc/backends/loopback.h
    alc/backends/null.cpp
    alc/backends/null.h
)

# Check ALSA backend
option(ALSOFT_REQUIRE_ALSA "Require ALSA backend" OFF)
find_package(ALSA)
if(ALSA_FOUND)
    option(ALSOFT_BACKEND_ALSA "Enable ALSA backend" ON)
    if(ALSOFT_BACKEND_ALSA)
        set(HAVE_ALSA 1)
        set(BACKENDS  "${BACKENDS} ALSA${IS_LINKED},")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/alsa.cpp alc/backends/alsa.h)
        add_backend_libs(${ALSA_LIBRARIES})
        set(INC_PATHS ${INC_PATHS} ${ALSA_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_ALSA AND NOT HAVE_ALSA)
    message(FATAL_ERROR "Failed to enabled required ALSA backend")
endif()

# Check OSS backend
option(ALSOFT_REQUIRE_OSS "Require OSS backend" OFF)
find_package(OSS)
if(OSS_FOUND)
    option(ALSOFT_BACKEND_OSS "Enable OSS backend" ON)
    if(ALSOFT_BACKEND_OSS)
        set(HAVE_OSS 1)
        set(BACKENDS  "${BACKENDS} OSS,")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/oss.cpp alc/backends/oss.h)
        if(OSS_LIBRARIES)
            set(EXTRA_LIBS ${OSS_LIBRARIES} ${EXTRA_LIBS})
        endif()
        set(INC_PATHS ${INC_PATHS} ${OSS_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_OSS AND NOT HAVE_OSS)
    message(FATAL_ERROR "Failed to enabled required OSS backend")
endif()

# Check PipeWire backend
option(ALSOFT_REQUIRE_PIPEWIRE "Require PipeWire backend" OFF)
if(PkgConfig_FOUND)
    pkg_check_modules(PIPEWIRE libpipewire-0.3)
    if(PIPEWIRE_FOUND)
        option(ALSOFT_BACKEND_PIPEWIRE "Enable PipeWire backend" ON)
        if(ALSOFT_BACKEND_PIPEWIRE)
            set(HAVE_PIPEWIRE 1)
            set(BACKENDS  "${BACKENDS} PipeWire${IS_LINKED},")
            set(ALC_OBJS  ${ALC_OBJS} alc/backends/pipewire.cpp alc/backends/pipewire.h)
            add_backend_libs(${PIPEWIRE_LIBRARIES})
            set(INC_PATHS ${INC_PATHS} ${PIPEWIRE_INCLUDE_DIRS})
        endif()
    endif()
endif()
if(ALSOFT_REQUIRE_PIPEWIRE AND NOT HAVE_PIPEWIRE)
    message(FATAL_ERROR "Failed to enabled required PipeWire backend")
endif()

# Check Solaris backend
option(ALSOFT_REQUIRE_SOLARIS "Require Solaris backend" OFF)
find_package(AudioIO)
if(AUDIOIO_FOUND)
    option(ALSOFT_BACKEND_SOLARIS "Enable Solaris backend" ON)
    if(ALSOFT_BACKEND_SOLARIS)
        set(HAVE_SOLARIS 1)
        set(BACKENDS  "${BACKENDS} Solaris,")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/solaris.cpp alc/backends/solaris.h)
        set(INC_PATHS ${INC_PATHS} ${AUDIOIO_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_SOLARIS AND NOT HAVE_SOLARIS)
    message(FATAL_ERROR "Failed to enabled required Solaris backend")
endif()

# Check SndIO backend
option(ALSOFT_REQUIRE_SNDIO "Require SndIO backend" OFF)
find_package(SoundIO)
if(SOUNDIO_FOUND)
    option(ALSOFT_BACKEND_SNDIO "Enable SndIO backend" ON)
    if(ALSOFT_BACKEND_SNDIO)
        set(HAVE_SNDIO 1)
        set(BACKENDS  "${BACKENDS} SndIO (linked),")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/sndio.cpp alc/backends/sndio.h)
        set(EXTRA_LIBS ${SOUNDIO_LIBRARIES} ${EXTRA_LIBS})
        set(INC_PATHS ${INC_PATHS} ${SOUNDIO_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_SNDIO AND NOT HAVE_SNDIO)
    message(FATAL_ERROR "Failed to enabled required SndIO backend")
endif()

# Check Windows-only backends
option(ALSOFT_REQUIRE_WINMM "Require Windows Multimedia backend" OFF)
option(ALSOFT_REQUIRE_DSOUND "Require DirectSound backend" OFF)
option(ALSOFT_REQUIRE_WASAPI "Require WASAPI backend" OFF)
if(WIN32)
    # Check MMSystem backend
    option(ALSOFT_BACKEND_WINMM "Enable Windows Multimedia backend" ON)
    if(ALSOFT_BACKEND_WINMM)
        set(HAVE_WINMM 1)
        set(BACKENDS "${BACKENDS} WinMM,")
        set(ALC_OBJS ${ALC_OBJS} alc/backends/winmm.cpp alc/backends/winmm.h)
        # There doesn't seem to be good way to search for winmm.lib for MSVC.
        # find_library doesn't find it without being told to look in a specific
        # place in the WindowsSDK, but it links anyway. If there ends up being
        # Windows targets without this, another means to detect it is needed.
        set(EXTRA_LIBS winmm ${EXTRA_LIBS})
    endif()

    # Check DSound backend
    check_include_file(dsound.h HAVE_DSOUND_H)
    if(DXSDK_DIR)
        find_path(DSOUND_INCLUDE_DIR NAMES "dsound.h"
            PATHS "${DXSDK_DIR}" PATH_SUFFIXES include
            DOC "The DirectSound include directory")
    endif()
    if(HAVE_DSOUND_H OR DSOUND_INCLUDE_DIR)
        option(ALSOFT_BACKEND_DSOUND "Enable DirectSound backend" ON)
        if(ALSOFT_BACKEND_DSOUND)
            set(HAVE_DSOUND 1)
            set(BACKENDS "${BACKENDS} DirectSound,")
            set(ALC_OBJS ${ALC_OBJS} alc/backends/dsound.cpp alc/backends/dsound.h)

            if(NOT HAVE_DSOUND_H)
                set(INC_PATHS ${INC_PATHS} ${DSOUND_INCLUDE_DIR})
            endif()
        endif()
    endif()

    # Check for WASAPI backend
    check_include_file(mmdeviceapi.h HAVE_MMDEVICEAPI_H)
    if(HAVE_MMDEVICEAPI_H)
        option(ALSOFT_BACKEND_WASAPI "Enable WASAPI backend" ON)
        if(ALSOFT_BACKEND_WASAPI)
            set(HAVE_WASAPI 1)
            set(BACKENDS  "${BACKENDS} WASAPI,")
            set(ALC_OBJS  ${ALC_OBJS} alc/backends/wasapi.cpp alc/backends/wasapi.h)
        endif()
    endif()
endif()
if(ALSOFT_REQUIRE_WINMM AND NOT HAVE_WINMM)
    message(FATAL_ERROR "Failed to enabled required WinMM backend")
endif()
if(ALSOFT_REQUIRE_DSOUND AND NOT HAVE_DSOUND)
    message(FATAL_ERROR "Failed to enabled required DSound backend")
endif()
if(ALSOFT_REQUIRE_WASAPI AND NOT HAVE_WASAPI)
    message(FATAL_ERROR "Failed to enabled required WASAPI backend")
endif()

# Check PortAudio backend
option(ALSOFT_REQUIRE_PORTAUDIO "Require PortAudio backend" OFF)
find_package(PortAudio)
if(PORTAUDIO_FOUND)
    option(ALSOFT_BACKEND_PORTAUDIO "Enable PortAudio backend" ON)
    if(ALSOFT_BACKEND_PORTAUDIO)
        set(HAVE_PORTAUDIO 1)
        set(BACKENDS  "${BACKENDS} PortAudio${IS_LINKED},")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/portaudio.cpp alc/backends/portaudio.h)
        add_backend_libs(${PORTAUDIO_LIBRARIES})
        set(INC_PATHS ${INC_PATHS} ${PORTAUDIO_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_PORTAUDIO AND NOT HAVE_PORTAUDIO)
    message(FATAL_ERROR "Failed to enabled required PortAudio backend")
endif()

# Check PulseAudio backend
option(ALSOFT_REQUIRE_PULSEAUDIO "Require PulseAudio backend" OFF)
find_package(PulseAudio)
if(PULSEAUDIO_FOUND)
    option(ALSOFT_BACKEND_PULSEAUDIO "Enable PulseAudio backend" ON)
    if(ALSOFT_BACKEND_PULSEAUDIO)
        set(HAVE_PULSEAUDIO 1)
        set(BACKENDS  "${BACKENDS} PulseAudio${IS_LINKED},")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/pulseaudio.cpp alc/backends/pulseaudio.h)
        add_backend_libs(${PULSEAUDIO_LIBRARIES})
        set(INC_PATHS ${INC_PATHS} ${PULSEAUDIO_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_PULSEAUDIO AND NOT HAVE_PULSEAUDIO)
    message(FATAL_ERROR "Failed to enabled required PulseAudio backend")
endif()

# Check JACK backend
option(ALSOFT_REQUIRE_JACK "Require JACK backend" OFF)
find_package(JACK)
if(JACK_FOUND)
    option(ALSOFT_BACKEND_JACK "Enable JACK backend" ON)
    if(ALSOFT_BACKEND_JACK)
        set(HAVE_JACK 1)
        set(BACKENDS  "${BACKENDS} JACK${IS_LINKED},")
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/jack.cpp alc/backends/jack.h)
        add_backend_libs(${JACK_LIBRARIES})
        set(INC_PATHS ${INC_PATHS} ${JACK_INCLUDE_DIRS})
    endif()
endif()
if(ALSOFT_REQUIRE_JACK AND NOT HAVE_JACK)
    message(FATAL_ERROR "Failed to enabled required JACK backend")
endif()

# Check CoreAudio backend
option(ALSOFT_REQUIRE_COREAUDIO "Require CoreAudio backend" OFF)
find_library(COREAUDIO_FRAMEWORK NAMES CoreAudio)
find_path(AUDIOUNIT_INCLUDE_DIR NAMES AudioUnit/AudioUnit.h)
if(COREAUDIO_FRAMEWORK AND AUDIOUNIT_INCLUDE_DIR)
    option(ALSOFT_BACKEND_COREAUDIO "Enable CoreAudio backend" ON)
    if(ALSOFT_BACKEND_COREAUDIO)
        set(HAVE_COREAUDIO 1)
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/coreaudio.cpp alc/backends/coreaudio.h)
        set(BACKENDS  "${BACKENDS} CoreAudio,")
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
            set(EXTRA_LIBS ${COREAUDIO_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK} ${EXTRA_LIBS})
        else()
            set(EXTRA_LIBS ${COREAUDIO_FRAMEWORK} /System/Library/Frameworks/AudioUnit.framework
                /System/Library/Frameworks/ApplicationServices.framework ${EXTRA_LIBS})
        endif()

        # Some versions of OSX may need the AudioToolbox framework. Add it if
        # it's found.
        find_library(AUDIOTOOLBOX_LIBRARY NAMES AudioToolbox)
        if(AUDIOTOOLBOX_LIBRARY)
            set(EXTRA_LIBS ${AUDIOTOOLBOX_LIBRARY} ${EXTRA_LIBS})
        endif()

        set(INC_PATHS ${INC_PATHS} ${AUDIOUNIT_INCLUDE_DIR})
    endif()
endif()
if(ALSOFT_REQUIRE_COREAUDIO AND NOT HAVE_COREAUDIO)
    message(FATAL_ERROR "Failed to enabled required CoreAudio backend")
endif()

# Check for OpenSL (Android) backend
option(ALSOFT_REQUIRE_OPENSL "Require OpenSL backend" OFF)
find_package(OpenSL)
if(OPENSL_FOUND)
    option(ALSOFT_BACKEND_OPENSL "Enable OpenSL backend" ON)
    if(ALSOFT_BACKEND_OPENSL)
        set(HAVE_OPENSL 1)
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/opensl.cpp alc/backends/opensl.h)
        set(BACKENDS  "${BACKENDS} OpenSL,")
        set(EXTRA_LIBS "OpenSL::OpenSLES" ${EXTRA_LIBS})
    endif()
endif()
if(ALSOFT_REQUIRE_OPENSL AND NOT HAVE_OPENSL)
    message(FATAL_ERROR "Failed to enabled required OpenSL backend")
endif()

# Check for Oboe (Android) backend
set(OBOE_TARGET )
if(ANDROID)
    set(OBOE_SOURCE "" CACHE STRING "Source directory for Oboe.")
    if(OBOE_SOURCE)
        # Force Oboe to build with hidden symbols. Don't want to be exporting
        # them from OpenAL.
        set(OLD_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        check_cxx_compiler_flag(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN_SWITCH)
        if(HAVE_VISIBILITY_HIDDEN_SWITCH)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
        endif()
        add_subdirectory(${OBOE_SOURCE} ./oboe)
        set(CMAKE_CXX_FLAGS ${OLD_CXX_FLAGS})
        unset(OLD_CXX_FLAGS)

        set(OBOE_TARGET oboe)
    else()
        find_package(Oboe)
        if(OBOE_FOUND)
            set(OBOE_TARGET "oboe::oboe")
        endif()
    endif()
endif()

option(ALSOFT_REQUIRE_OBOE "Require Oboe backend" OFF)
if(OBOE_TARGET)
    option(ALSOFT_BACKEND_OBOE "Enable Oboe backend" ON)
    if(ALSOFT_BACKEND_OBOE)
        set(HAVE_OBOE 1)
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/oboe.cpp alc/backends/oboe.h)
        set(BACKENDS  "${BACKENDS} Oboe,")
        set(EXTRA_LIBS ${OBOE_TARGET} ${EXTRA_LIBS})
    endif()
endif()
if(ALSOFT_REQUIRE_OBOE AND NOT HAVE_OBOE)
    message(FATAL_ERROR "Failed to enabled required Oboe backend")
endif()

# Check for SDL2 backend
option(ALSOFT_REQUIRE_SDL2 "Require SDL2 backend" OFF)
find_package(SDL2)
if(SDL2_FOUND)
    # Off by default, since it adds a runtime dependency
    option(ALSOFT_BACKEND_SDL2 "Enable SDL2 backend" OFF)
    if(ALSOFT_BACKEND_SDL2)
        set(HAVE_SDL2 1)
        set(ALC_OBJS  ${ALC_OBJS} alc/backends/sdl2.cpp alc/backends/sdl2.h)
        set(BACKENDS  "${BACKENDS} SDL2,")
        set(EXTRA_LIBS ${SDL2_LIBRARY} ${EXTRA_LIBS})
        set(INC_PATHS ${INC_PATHS} ${SDL2_INCLUDE_DIR})
    endif()
endif()
if(ALSOFT_REQUIRE_SDL2 AND NOT SDL2_FOUND)
    message(FATAL_ERROR "Failed to enabled required SDL2 backend")
endif()

# Optionally enable the Wave Writer backend
option(ALSOFT_BACKEND_WAVE "Enable Wave Writer backend" ON)
if(ALSOFT_BACKEND_WAVE)
    set(HAVE_WAVE 1)
    set(ALC_OBJS  ${ALC_OBJS} alc/backends/wave.cpp alc/backends/wave.h)
    set(BACKENDS  "${BACKENDS} WaveFile,")
endif()

# This is always available
set(BACKENDS  "${BACKENDS} Null")


find_package(Git)
if(ALSOFT_UPDATE_BUILD_VERSION AND GIT_FOUND AND EXISTS "${OpenAL_SOURCE_DIR}/.git")
    # Get the current working branch and its latest abbreviated commit hash
    add_custom_target(build_version
        ${CMAKE_COMMAND} -D GIT_EXECUTABLE=${GIT_EXECUTABLE} -D LIB_VERSION=${LIB_VERSION}
            -D LIB_VERSION_NUM=${LIB_VERSION_NUM} -D SRC=${OpenAL_SOURCE_DIR}/version.h.in
            -D DST=${OpenAL_BINARY_DIR}/version.h -P ${OpenAL_SOURCE_DIR}/version.cmake
        WORKING_DIRECTORY "${OpenAL_SOURCE_DIR}"
        VERBATIM
    )
else()
    set(GIT_BRANCH "UNKNOWN")
    set(GIT_COMMIT_HASH "unknown")
    configure_file(
        "${OpenAL_SOURCE_DIR}/version.h.in"
        "${OpenAL_BINARY_DIR}/version.h")
endif()


option(ALSOFT_EMBED_HRTF_DATA "Embed the HRTF data files (increases library footprint)" ON)
if(ALSOFT_EMBED_HRTF_DATA)
    macro(make_hrtf_header FILENAME VARNAME)
        set(infile  "${OpenAL_SOURCE_DIR}/hrtf/${FILENAME}")
        set(outfile  "${OpenAL_BINARY_DIR}/${VARNAME}.h")

        add_custom_command(OUTPUT "${outfile}"
            COMMAND ${CMAKE_COMMAND} -D "INPUT_FILE=${infile}" -D "OUTPUT_FILE=${outfile}"
                -D "VARIABLE_NAME=${VARNAME}" -P "${CMAKE_MODULE_PATH}/bin2h.script.cmake"
            WORKING_DIRECTORY "${OpenAL_SOURCE_DIR}"
            DEPENDS "${infile}" "${CMAKE_MODULE_PATH}/bin2h.script.cmake"
            VERBATIM
        )
        set(ALC_OBJS  ${ALC_OBJS} "${outfile}")
    endmacro()

    make_hrtf_header("Default HRTF.mhr" "hrtf_default")
endif()


if(ALSOFT_UTILS)
    find_package(MySOFA)
    if(NOT ALSOFT_NO_CONFIG_UTIL)
        find_package(Qt5Widgets QUIET)
        if(NOT Qt5Widgets_FOUND)
            message(STATUS "Could NOT find Qt5Widgets")
        endif()
    endif()
endif()
if(ALSOFT_UTILS OR ALSOFT_EXAMPLES)
    find_package(SndFile)
    find_package(SDL2)
    if(SDL2_FOUND)
        find_package(FFmpeg COMPONENTS AVFORMAT AVCODEC AVUTIL SWSCALE SWRESAMPLE)
    endif()
endif()

if(NOT WIN32)
    set(LIBNAME "openal")
else()
    set(LIBNAME "OpenAL32")
endif()

# Needed for openal.pc.in
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
set(bindir "\${exec_prefix}/${CMAKE_INSTALL_BINDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(PACKAGE_VERSION "${LIB_VERSION}")
set(PKG_CONFIG_CFLAGS )
set(PKG_CONFIG_PRIVATE_LIBS )
if(LIBTYPE STREQUAL "STATIC")
    set(PKG_CONFIG_CFLAGS -DAL_LIBTYPE_STATIC)
    foreach(FLAG  ${LINKER_FLAGS} ${EXTRA_LIBS} ${MATH_LIB})
        # If this is already a linker flag, or is a full path+file, add it
        # as-is. Otherwise, it's a name intended to be dressed as -lname.
        if(FLAG MATCHES "^-.*" OR EXISTS "${FLAG}")
            set(PKG_CONFIG_PRIVATE_LIBS "${PKG_CONFIG_PRIVATE_LIBS} ${FLAG}")
        else()
            set(PKG_CONFIG_PRIVATE_LIBS "${PKG_CONFIG_PRIVATE_LIBS} -l${FLAG}")
        endif()
    endforeach()
endif()

# End configuration
configure_file(
    "${OpenAL_SOURCE_DIR}/config.h.in"
    "${OpenAL_BINARY_DIR}/config.h")
configure_file(
    "${OpenAL_SOURCE_DIR}/openal.pc.in"
    "${OpenAL_BINARY_DIR}/openal.pc"
    @ONLY)


add_library(common STATIC EXCLUDE_FROM_ALL ${COMMON_OBJS})
target_include_directories(common PRIVATE ${OpenAL_BINARY_DIR} ${OpenAL_SOURCE_DIR}/include)
target_compile_definitions(common PRIVATE ${CPP_DEFS})
target_compile_options(common PRIVATE ${C_FLAGS})
set_target_properties(common PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


unset(HAS_ROUTER)
set(IMPL_TARGET OpenAL) # Either OpenAL or soft_oal.

# Build main library
if(LIBTYPE STREQUAL "STATIC")
    add_library(${IMPL_TARGET} STATIC ${COMMON_OBJS} ${OPENAL_OBJS} ${ALC_OBJS} ${CORE_OBJS})
    target_compile_definitions(${IMPL_TARGET} PUBLIC AL_LIBTYPE_STATIC)
    target_link_libraries(${IMPL_TARGET} PRIVATE ${LINKER_FLAGS} ${EXTRA_LIBS} ${MATH_LIB})

    if(WIN32)
        # This option is for static linking OpenAL Soft into another project
        # that already defines the IDs. It is up to that project to ensure all
        # required IDs are defined.
        option(ALSOFT_NO_UID_DEFS "Do not define GUIDs, IIDs, CLSIDs, or PropertyKeys" OFF)
        if(ALSOFT_NO_UID_DEFS)
            target_compile_definitions(${IMPL_TARGET} PRIVATE AL_NO_UID_DEFS)
        endif()
    endif()
else()
    set(RC_CONFIG resources/openal32.rc)
    if(WIN32 AND ALSOFT_BUILD_ROUTER)
        add_library(OpenAL SHARED
            resources/router.rc
            router/router.cpp
            router/router.h
            router/alc.cpp
            router/al.cpp
        )
        target_compile_definitions(OpenAL
            PRIVATE AL_BUILD_LIBRARY AL_ALEXT_PROTOTYPES "ALC_API=${EXPORT_DECL}"
            "AL_API=${EXPORT_DECL}" ${CPP_DEFS})
        target_compile_options(OpenAL PRIVATE ${C_FLAGS})
        target_link_libraries(OpenAL PRIVATE common ${LINKER_FLAGS})
        target_include_directories(OpenAL
          PUBLIC
            $<BUILD_INTERFACE:${OpenAL_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
          PRIVATE
            ${OpenAL_SOURCE_DIR}/common
            ${OpenAL_BINARY_DIR}
        )
        set_target_properties(OpenAL PROPERTIES PREFIX "")
        set_target_properties(OpenAL PROPERTIES OUTPUT_NAME ${LIBNAME})
        if(TARGET build_version)
            add_dependencies(OpenAL build_version)
        endif()
        set(HAS_ROUTER 1)

        set(LIBNAME "soft_oal")
        set(IMPL_TARGET soft_oal)
        set(RC_CONFIG resources/soft_oal.rc)
    endif()

    # !important: for osx framework public header works, the headers must be in
    # the project
    set(TARGET_PUBLIC_HEADERS include/AL/al.h include/AL/alc.h include/AL/alext.h include/AL/efx.h)
    add_library(${IMPL_TARGET} SHARED ${OPENAL_OBJS} ${ALC_OBJS} ${CORE_OBJS} ${RC_CONFIG}
        ${TARGET_PUBLIC_HEADERS})
    if(WIN32)
        set_target_properties(${IMPL_TARGET} PROPERTIES PREFIX "")
    endif()
    target_link_libraries(${IMPL_TARGET} PRIVATE common ${LINKER_FLAGS} ${EXTRA_LIBS} ${MATH_LIB})

    if(APPLE AND ALSOFT_OSX_FRAMEWORK)
        # Sets framework name to soft_oal to avoid ambiguity with the system OpenAL.framework
        set(LIBNAME "soft_oal")
        if(GIT_FOUND)
            EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} rev-list --count head
                TIMEOUT 5
                OUTPUT_VARIABLE BUNDLE_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        else()
            set(BUNDLE_VERSION 1)
        endif()

        # Build: Fix rpath in iOS shared libraries
        # If this flag is not set, the final dylib binary ld path will be
        # /User/xxx/*/soft_oal.framework/soft_oal, which can't be loaded by iOS devices.
        # See also: https://github.com/libjpeg-turbo/libjpeg-turbo/commit/c80ddef7a4ce21ace9e3ca0fd190d320cc8cdaeb
        if(NOT CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG)
            set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
        endif()

        set_target_properties(${IMPL_TARGET} PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION C
            MACOSX_FRAMEWORK_NAME "${IMPL_TARGET}"
            MACOSX_FRAMEWORK_IDENTIFIER "org.openal-soft.openal"
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${LIB_VERSION}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${BUNDLE_VERSION}
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
            PUBLIC_HEADER "${TARGET_PUBLIC_HEADERS}"
            MACOSX_RPATH TRUE)
    endif()
endif()

target_include_directories(${IMPL_TARGET}
  PUBLIC
    $<BUILD_INTERFACE:${OpenAL_SOURCE_DIR}/include>
  INTERFACE
    $<BUILD_INTERFACE:${OpenAL_SOURCE_DIR}/include/AL>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/AL>
  PRIVATE
    ${INC_PATHS}
    ${OpenAL_BINARY_DIR}
    ${OpenAL_SOURCE_DIR}
    ${OpenAL_SOURCE_DIR}/common
)

set_target_properties(${IMPL_TARGET} PROPERTIES OUTPUT_NAME ${LIBNAME}
    VERSION ${LIB_VERSION}
    SOVERSION ${LIB_MAJOR_VERSION}
)
target_compile_definitions(${IMPL_TARGET}
    PRIVATE AL_BUILD_LIBRARY AL_ALEXT_PROTOTYPES "ALC_API=${EXPORT_DECL}" "AL_API=${EXPORT_DECL}"
    ${CPP_DEFS})
target_compile_options(${IMPL_TARGET} PRIVATE ${C_FLAGS})

if(TARGET build_version)
    add_dependencies(${IMPL_TARGET} build_version)
endif()

if(WIN32 AND MINGW AND ALSOFT_BUILD_IMPORT_LIB AND NOT LIBTYPE STREQUAL "STATIC")
    find_program(SED_EXECUTABLE NAMES sed DOC "sed executable")
    if(NOT SED_EXECUTABLE OR NOT CMAKE_DLLTOOL)
        message(STATUS "")
        if(NOT SED_EXECUTABLE)
            message(STATUS "WARNING: Cannot find sed, disabling .def/.lib generation")
        endif()
        if(NOT CMAKE_DLLTOOL)
            message(STATUS "WARNING: Cannot find dlltool, disabling .def/.lib generation")
        endif()
    else()
        set_property(TARGET OpenAL APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,--output-def,OpenAL32.def")
        add_custom_command(TARGET OpenAL POST_BUILD
            COMMAND "${SED_EXECUTABLE}" -i -e "s/ @[^ ]*//" OpenAL32.def
            COMMAND "${CMAKE_DLLTOOL}" -d OpenAL32.def -l OpenAL32.lib -D OpenAL32.dll
            COMMENT "Stripping ordinals from OpenAL32.def and generating OpenAL32.lib..."
            VERBATIM
        )
    endif()
endif()

if(HAS_ROUTER)
    message(STATUS "")
    message(STATUS "Building DLL router")
endif()

message(STATUS "")
message(STATUS "Building OpenAL with support for the following backends:")
message(STATUS "   ${BACKENDS}")
message(STATUS "")
message(STATUS "Building with support for CPU extensions:")
message(STATUS "    ${CPU_EXTS}")
message(STATUS "")
if(FPMATH_SET)
    message(STATUS "Building with SSE${FPMATH_SET} codegen")
    message(STATUS "")
endif()

if(ALSOFT_EAX)
    message(STATUS "Building with legacy EAX extension support")
    message(STATUS "")
endif()

if(ALSOFT_EMBED_HRTF_DATA)
    message(STATUS "Embedding HRTF datasets")
    message(STATUS "")
endif()

# An alias for sub-project builds
add_library(OpenAL::OpenAL ALIAS OpenAL)

# Install main library
if(ALSOFT_INSTALL)
    configure_package_config_file(OpenALConfig.cmake.in OpenALConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenAL)
    install(TARGETS OpenAL EXPORT OpenAL
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} ${CMAKE_INSTALL_INCLUDEDIR}/AL)
    export(TARGETS OpenAL
        NAMESPACE OpenAL::
        FILE OpenALTargets.cmake)
    install(EXPORT OpenAL
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenAL
        NAMESPACE OpenAL::
        FILE OpenALTargets.cmake)
    install(DIRECTORY include/AL
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES "${OpenAL_BINARY_DIR}/openal.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    install(FILES "${OpenAL_BINARY_DIR}/OpenALConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/OpenAL")
    if(TARGET soft_oal)
        install(TARGETS soft_oal
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    message(STATUS "Installing library and headers")
else()
    message(STATUS "NOT installing library and headers")
endif()

if(ALSOFT_INSTALL_CONFIG)
    install(FILES alsoftrc.sample
        DESTINATION ${CMAKE_INSTALL_DATADIR}/openal)
    message(STATUS "Installing sample configuration")
endif()

if(ALSOFT_INSTALL_HRTF_DATA)
    install(DIRECTORY hrtf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/openal)
    message(STATUS "Installing HRTF data files")
endif()

if(ALSOFT_INSTALL_AMBDEC_PRESETS)
    install(DIRECTORY presets
        DESTINATION ${CMAKE_INSTALL_DATADIR}/openal)
    message(STATUS "Installing AmbDec presets")
endif()
message(STATUS "")

set(UNICODE_FLAG )
if(MINGW)
    set(UNICODE_FLAG ${UNICODE_FLAG} -municode)
endif()
set(EXTRA_INSTALLS )
if(ALSOFT_UTILS)
    add_executable(openal-info utils/openal-info.c)
    target_include_directories(openal-info PRIVATE ${OpenAL_SOURCE_DIR}/common)
    target_compile_options(openal-info PRIVATE ${C_FLAGS})
    target_link_libraries(openal-info PRIVATE ${LINKER_FLAGS} OpenAL ${UNICODE_FLAG})
    if(ALSOFT_INSTALL_EXAMPLES)
        set(EXTRA_INSTALLS ${EXTRA_INSTALLS} openal-info)
    endif()

    if(SNDFILE_FOUND)
        add_executable(uhjdecoder utils/uhjdecoder.cpp)
        target_compile_definitions(uhjdecoder PRIVATE ${CPP_DEFS})
        target_include_directories(uhjdecoder
            PRIVATE ${OpenAL_BINARY_DIR} ${OpenAL_SOURCE_DIR}/common)
        target_compile_options(uhjdecoder PRIVATE ${C_FLAGS})
        target_link_libraries(uhjdecoder PUBLIC common
            PRIVATE ${LINKER_FLAGS} SndFile::SndFile ${UNICODE_FLAG})

        add_executable(uhjencoder utils/uhjencoder.cpp)
        target_compile_definitions(uhjencoder PRIVATE ${CPP_DEFS})
        target_include_directories(uhjencoder
            PRIVATE ${OpenAL_BINARY_DIR} ${OpenAL_SOURCE_DIR}/common)
        target_compile_options(uhjencoder PRIVATE ${C_FLAGS})
        target_link_libraries(uhjencoder PUBLIC common
            PRIVATE ${LINKER_FLAGS} SndFile::SndFile ${UNICODE_FLAG})
    endif()

    if(MYSOFA_FOUND)
        set(SOFA_SUPPORT_SRCS
            utils/sofa-support.cpp
            utils/sofa-support.h)
        add_library(sofa-support STATIC EXCLUDE_FROM_ALL ${SOFA_SUPPORT_SRCS})
        target_compile_definitions(sofa-support PRIVATE ${CPP_DEFS})
        target_include_directories(sofa-support PUBLIC ${OpenAL_SOURCE_DIR}/common)
        target_compile_options(sofa-support PRIVATE ${C_FLAGS})
        target_link_libraries(sofa-support PUBLIC common MySOFA::MySOFA PRIVATE ${LINKER_FLAGS})

        set(MAKEMHR_SRCS
            utils/makemhr/loaddef.cpp
            utils/makemhr/loaddef.h
            utils/makemhr/loadsofa.cpp
            utils/makemhr/loadsofa.h
            utils/makemhr/makemhr.cpp
            utils/makemhr/makemhr.h)
        if(NOT HAVE_GETOPT)
            set(MAKEMHR_SRCS  ${MAKEMHR_SRCS} utils/getopt.c utils/getopt.h)
        endif()
        add_executable(makemhr ${MAKEMHR_SRCS})
        target_compile_definitions(makemhr PRIVATE ${CPP_DEFS})
        target_include_directories(makemhr
            PRIVATE ${OpenAL_BINARY_DIR} ${OpenAL_SOURCE_DIR}/utils)
        target_compile_options(makemhr PRIVATE ${C_FLAGS})
        target_link_libraries(makemhr PRIVATE ${LINKER_FLAGS} sofa-support ${UNICODE_FLAG})
        if(ALSOFT_INSTALL_EXAMPLES)
            set(EXTRA_INSTALLS ${EXTRA_INSTALLS} makemhr)
        endif()

        set(SOFAINFO_SRCS  utils/sofa-info.cpp)
        add_executable(sofa-info ${SOFAINFO_SRCS})
        target_compile_definitions(sofa-info PRIVATE ${CPP_DEFS})
        target_include_directories(sofa-info PRIVATE ${OpenAL_SOURCE_DIR}/utils)
        target_compile_options(sofa-info PRIVATE ${C_FLAGS})
        target_link_libraries(sofa-info PRIVATE ${LINKER_FLAGS} sofa-support ${UNICODE_FLAG})
    endif()
    message(STATUS "Building utility programs")

    if(NOT ALSOFT_NO_CONFIG_UTIL)
        add_subdirectory(utils/alsoft-config)
    endif()
    message(STATUS "")
endif()


# Add a static library with common functions used by multiple example targets
add_library(ex-common STATIC EXCLUDE_FROM_ALL
    examples/common/alhelpers.c
    examples/common/alhelpers.h)
target_compile_definitions(ex-common PUBLIC ${CPP_DEFS})
target_include_directories(ex-common PUBLIC ${OpenAL_SOURCE_DIR}/common)
target_compile_options(ex-common PUBLIC ${C_FLAGS})
target_link_libraries(ex-common PUBLIC OpenAL PRIVATE ${RT_LIB})

if(ALSOFT_EXAMPLES)
    add_executable(altonegen examples/altonegen.c)
    target_link_libraries(altonegen PRIVATE ${LINKER_FLAGS} ${MATH_LIB} ex-common ${UNICODE_FLAG})

    add_executable(alrecord examples/alrecord.c)
    target_link_libraries(alrecord PRIVATE ${LINKER_FLAGS} ex-common ${UNICODE_FLAG})

    if(ALSOFT_INSTALL_EXAMPLES)
        set(EXTRA_INSTALLS ${EXTRA_INSTALLS} altonegen alrecord)
    endif()

    message(STATUS "Building example programs")

    if(SNDFILE_FOUND)
        add_executable(alplay examples/alplay.c)
        target_link_libraries(alplay PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common
            ${UNICODE_FLAG})

        add_executable(alstream examples/alstream.c)
        target_link_libraries(alstream PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common
            ${UNICODE_FLAG})

        add_executable(alreverb examples/alreverb.c)
        target_link_libraries(alreverb PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common
            ${UNICODE_FLAG})

        add_executable(almultireverb examples/almultireverb.c)
        target_link_libraries(almultireverb
            PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common ${MATH_LIB} ${UNICODE_FLAG})

        add_executable(allatency examples/allatency.c)
        target_link_libraries(allatency PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common
            ${UNICODE_FLAG})

        add_executable(alhrtf examples/alhrtf.c)
        target_link_libraries(alhrtf
            PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common ${MATH_LIB} ${UNICODE_FLAG})

        add_executable(alstreamcb examples/alstreamcb.cpp)
        target_link_libraries(alstreamcb PRIVATE ${LINKER_FLAGS} SndFile::SndFile ex-common
            ${UNICODE_FLAG})

        add_executable(alconvolve examples/alconvolve.c)
        target_link_libraries(alconvolve PRIVATE ${LINKER_FLAGS} common SndFile::SndFile ex-common
            ${UNICODE_FLAG})

        if(ALSOFT_INSTALL_EXAMPLES)
            set(EXTRA_INSTALLS ${EXTRA_INSTALLS} alplay alstream alreverb almultireverb allatency
                alhrtf)
        endif()

        message(STATUS "Building SndFile example programs")
    endif()

    if(SDL2_FOUND)
        add_executable(alloopback examples/alloopback.c)
        target_include_directories(alloopback PRIVATE ${SDL2_INCLUDE_DIR})
        target_link_libraries(alloopback
            PRIVATE ${LINKER_FLAGS} ${SDL2_LIBRARY} ex-common ${MATH_LIB})

        if(ALSOFT_INSTALL_EXAMPLES)
            set(EXTRA_INSTALLS ${EXTRA_INSTALLS} alloopback)
        endif()

        message(STATUS "Building SDL example programs")

        set(FFVER_OK FALSE)
        if(FFMPEG_FOUND)
            set(FFVER_OK TRUE)
            if(AVFORMAT_VERSION VERSION_LESS "57.56.101")
                message(STATUS "libavformat is too old! (${AVFORMAT_VERSION}, wanted 57.56.101)")
                set(FFVER_OK FALSE)
            endif()
            if(AVCODEC_VERSION VERSION_LESS "57.64.101")
                message(STATUS "libavcodec is too old! (${AVCODEC_VERSION}, wanted 57.64.101)")
                set(FFVER_OK FALSE)
            endif()
            if(AVUTIL_VERSION VERSION_LESS "55.34.101")
                message(STATUS "libavutil is too old! (${AVUTIL_VERSION}, wanted 55.34.101)")
                set(FFVER_OK FALSE)
            endif()
            if(SWSCALE_VERSION VERSION_LESS "4.2.100")
                message(STATUS "libswscale is too old! (${SWSCALE_VERSION}, wanted 4.2.100)")
                set(FFVER_OK FALSE)
            endif()
            if(SWRESAMPLE_VERSION VERSION_LESS "2.3.100")
                message(STATUS "libswresample is too old! (${SWRESAMPLE_VERSION}, wanted 2.3.100)")
                set(FFVER_OK FALSE)
            endif()
        endif()
        if(FFVER_OK)
            add_executable(alffplay examples/alffplay.cpp)
            target_include_directories(alffplay
                PRIVATE ${SDL2_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})
            target_link_libraries(alffplay
                PRIVATE ${LINKER_FLAGS} ${SDL2_LIBRARY} ${FFMPEG_LIBRARIES} ex-common)

            if(ALSOFT_INSTALL_EXAMPLES)
                set(EXTRA_INSTALLS ${EXTRA_INSTALLS} alffplay)
            endif()
            message(STATUS "Building SDL+FFmpeg example programs")
        endif()
    endif()
    message(STATUS "")
endif()

if(EXTRA_INSTALLS)
    install(TARGETS ${EXTRA_INSTALLS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

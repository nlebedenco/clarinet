cmake_minimum_required(VERSION 3.18)

# For project version variables
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

# Only interpret if() arguments as variables or keywords when unquoted.
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

# Honor visibility properties for all target types.
if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

# We want find_file() and find_library() to honor {packagename}_ROOT, as that appears to be the only way, with the
# Visual Studio 2019 IDE and its CMake support, to tell CMake where to look for dependencies.
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

# Allow top project to override option() with a variable. This is important so forced options don't appear in the cache
# and mislead the user into thinking they can be modified.
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# Enable MSVC_RUNTIME_LIBRARY property
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

# Set custom cmake modules path
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Set custom cmake helpers path
set (CMAKE_HELPER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Helpers/)

# Set name of folder used to group predefined targets in the Visual Studio solution
set(CMAKE_PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

# Make VisualStudio put ALL_BUILD, INSTALL and ZERO_CHECK projects in a separate solution folder.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ${CMAKE_PREDEFINED_TARGETS_FOLDER})

# If we're building with Visual Studio, we require at least Visual Studio 2015, in order to get sufficient C99
# compatibility.
if(MSVC AND MSVC_VERSION LESS 1900)
    message(FATAL_ERROR "Visual Studio 2015 or later is required")
endif()

# Windows DLLs go into RUNTIME_OUTPUT_DIRECTORY so executables can find their libraries. Set output directories here
# for all targets so we can find dynamic libraries when any of the executables are run.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib)


########################################################################################################################
#   Project
########################################################################################################################
# Project name is used for the main target name and file name.
# Version is exported in the DLL details (.rc file) and accessible to the user through the library API so numeric values
# cannot be greater than 255.
# Description is exported in the DLL details (.rc file) and accessible to the user through the library API.
project(clarinet
    VERSION 0.0.1
    DESCRIPTION "Experimental network transport protocol stack"
    HOMEPAGE_URL ""
)

# Require out-of-source builds. Technically the if(EXISTS ...) will return false if the file exists but is not 
# accessible by the user but in this case there is a good chance the directory is not writable either so something is 
# already wrong.
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" CMAKE_LISTS_FILE)
if(EXISTS "${CMAKE_LISTS_FILE}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# Set installation prefix automatically to custom path if not explicitly specified on the command line
if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT))
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE )
endif()

# Inform what we are configuring in case the output is stored in a log file.
message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")
get_directory_property(PROJECT_HAS_PARENT PARENT_DIRECTORY)
if(NOT PROJECT_HAS_PARENT)
    message(STATUS "Install prefix is ${CMAKE_INSTALL_PREFIX}")
endif()

# Define an upper case and capitalized project names.
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
string(SUBSTRING ${PROJECT_NAME} 0 1 FIRST_LETTER)
string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" PROJECT_NAME_CAPITALIZED "${PROJECT_NAME}")

# Define system variables
include(DefineSystemVariables)

# Define a project system name (lower case CMAKE_SYSTEM_NAME). This must go after including DefineSystemVariables 
# because CMAKE_SYSTEM_NAME might have been adjusted there (e.g. for cross-compilation).
string(TOLOWER ${CMAKE_SYSTEM_NAME} PROJECT_SYSTEM_NAME)

# Define original file name for dll details
if(WIN32)
    if(BUILD_SHARED_LIBS)
        set(PROJECT_FILENAME ${PROJECT_NAME}.dll)
    endif()
endif(WIN32)

########################################################################################################################
#   Options
########################################################################################################################

option(${PROJECT_NAME_UPPERCASE}_ENABLE_PROFILER    "Enable profiler"   OFF)
option(${PROJECT_NAME_UPPERCASE}_ENABLE_LOG         "Enable Log"        ON)
option(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6        "Enable IPv6"       ON)
option(${PROJECT_NAME_UPPERCASE}_ENABLE_UDP         "Enable UDP"        ON)
option(${PROJECT_NAME_UPPERCASE}_ENABLE_TCP         "Enable TCP"        ON)

include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6DUAL  "Enable IPv6 Dual Stack"  ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6"  OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_DTLS      "Enable DTLS"             ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_UDP"   OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_DTLC      "Enable DTLC"             ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_UDP"   OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_UDTP      "Enable UDTP"             ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_DTLC"  OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_UDTPS     "Enable UDTPS"            ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_DTLS"  OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_ENET      "Enable ENET"             ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_UDP"   OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_ENET      "Enable ENETS"            ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_DTLS"  OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME_UPPERCASE}_ENABLE_TLS       "Enable TLS"              ON  "${PROJECT_NAME_UPPERCASE}_ENABLE_TCP"   OFF)

option(${PROJECT_NAME_UPPERCASE}_USE_STATIC_RT          "Use static runtime"                                OFF)
option(${PROJECT_NAME_UPPERCASE}_USE_STACK_PROTECTION   "Use stack protection if supported by the compiler" ON)
option(${PROJECT_NAME_UPPERCASE}_USE_SPECTRE_MITIGATION "Use spectre mitigation if support by the compiler" OFF)

# Defaults to build testing only if this is the top cmake project. 
# The reason for this is that if someone else includes this package, and they use BUILD_TESTING, they probably do not 
# our tests to build. In the rare case that they really do want to enable testing on both packages, they can force 
# a cache override for this option.
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(DEFAULT_${PROJECT_NAME_UPPERCASE}_BUILD_TESTING ON)
else()
    set(DEFAULT_${PROJECT_NAME_UPPERCASE}_BUILD_TESTING OFF)
endif()    
option(${PROJECT_NAME_UPPERCASE}_BUILD_TESTING "Build ${PROJECT_NAME} tests" ${DEFAULT_${PROJECT_NAME_UPPERCASE}_BUILD_TESTING})

# Automatically used by cmake to change the default for add_library()
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)


########################################################################################################################
#   Macros
########################################################################################################################

# For checking if a compiler flag works and add to the target if it does.
macro(target_compile_option _target _scope _option)
    message(STATUS "Checking C compiler flag ${_option}")
    string(REPLACE "=" "-" _temp_option_variable ${_option})
    string(REGEX REPLACE "^-" "" _option_variable ${_temp_option_variable})
    check_c_compiler_flag("${_option}" ${_option_variable})
    if(${${_option_variable}})
        target_compile_options(${_target} ${_scope} ${_option})
    endif()
endmacro()

# Based on https://gist.github.com/likema/97841d77e5f384fbdb46629c5b429013
# by Like Ma
# Check ANSI C headers. Once done it will define STDC_HEADERS, HAVE_STDLIB_H, HAVE_STDARG_H, HAVE_STRING_H and
# HAVE_FLOAT_H, if they exist
macro(check_stdc_headers)
    if(NOT CMAKE_REQUIRED_QUIET)
        message (CHECK_START "Checking whether system has ANSI C header files")
    endif()
    check_include_files("stdio.h;stdlib.h;stdarg.h;string.h" STDC_HEADERS)
    if(STDC_HEADERS)
        message (CHECK_PASS "found")
        set(STDC_HEADERS 1 CACHE INTERNAL "System has ANSI C header files")
        set(HAVE_STDIO_H 1 CACHE INTERNAL "Have include stdio.h")
        set(HAVE_STDLIB_H 1 CACHE INTERNAL "Have include stdlib.h")
        set(HAVE_STDARG_H 1 CACHE INTERNAL "Have include stdarg.h")
        set(HAVE_STRING_H 1 CACHE INTERNAL "Have include string.h")
    else()
        message(CHECK_FAIL "not found")
        set(STDC_HEADERS 0 CACHE INTERNAL "System has ANSI C header files")
    endif()
endmacro()

# Profiler support.
macro(target_profiler _target)
    if(${PROJECT_NAME_UPPERCASE}_ENABLE_PROFILER)
        if(MSVC) # including MSVC/CL (clang-cl)
            target_link_options(${_target} PRIVATE /PROFILE)
        else()
            target_compile_options(${_target} PRIVATE -pg)
            target_link_options(${_target} PRIVATE -pg)
        endif()
    endif()
endmacro()

# Static link to the system runtime library.
macro(target_static_runtime _target)
    if(${PROJECT_NAME_UPPERCASE}_USE_STATIC_RT)
        if(MSVC) # including MSVC/CL (clang-cl)
            set_property(TARGET ${_target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        else()
            target_link_options(${_target} PRIVATE -static-libgcc)
        endif()
    endif()
endmacro()

# Runtime stack protection.
macro(target_stack_protection _target)
    if(${PROJECT_NAME_UPPERCASE}_USE_STACK_PROTECTION)
        if(MSVC) # including MSVC/CL (clang-cl)
            target_compile_options(${_target} PRIVATE /GS)
        else()
            # Use of -fstack-protector may cause segmentation faults in MinGW possibly on Cygwin too.
            # We perform the test regardless but won't accept a positive if WIN32 until this bug is fixed.
            check_c_compiler_flag(-fstack-protector-strong COMPILER_SUPPORTS_STACK_PROTECTOR_STRONG)
            if(COMPILER_SUPPORTS_STACK_PROTECTOR_STRONG AND NOT WIN32)
                target_compile_options(${_target} PRIVATE -fstack-protector-strong)
            endif()
        endif()
    else()
        if(MSVC) # including MSVC/CL (clang-cl)
            target_compile_options(${_target} PRIVATE /GS-)
        else()
            # Use of -fstack-protector may cause segmentation faults in MinGW possibly on Cygwin too.
            # We perform the test regardless but won't accept a positive if WIN32 until this bug is fixed.
            check_c_compiler_flag(-fno-stack-protector COMPILER_SUPPORTS_NO_STACK_PROTECTOR)
            if(COMPILER_SUPPORTS_NO_STACK_PROTECTOR AND NOT WIN32)
                target_compile_options(${_target} PRIVATE -fno-stack-protector-strong)
            endif()
        endif()
    endif()
endmacro()

# Runtime spectre mitigation
macro(target_spectre_mitigation _target)
    if(${PROJECT_NAME_UPPERCASE}_USE_SPECTRE_MITIGATION)
        if(MSVC) # including MSVC/CL (clang-cl)
            check_c_compiler_flag(/guard:cf COMPILER_SUPPORTS_GUARDCF)
            if(COMPILER_SUPPORTS_GUARDCF)
                target_compile_options(${_target} PRIVATE /guard:cf)
                # For now we assume the liker also supports the complementary flag
                target_link_options(${_target} PRIVATE /guard:cf)
            endif()
            check_c_compiler_flag(/Qspectre COMPILER_SUPPORTS_QSPECTRE)
            if(COMPILER_SUPPORTS_QSPECTRE)
                target_compile_options(${_target} PRIVATE /Qspectre)
                # For now we assume the liker also supports the complementary flag
                target_link_options(${_target} PRIVATE /DYNAMICBASE)
            endif()
        else()
            if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
                check_c_compiler_flag(-mretpoline COMPILER_SUPPORTS_RETPOLINE)
                if(COMPILER_SUPPORTS_RETPOLINE)
                    target_compile_options(${_target} PRIVATE -mretpoline)
                endif()
            else()
                check_c_compiler_flag(-mindirect-branch=thunk COMPILER_SUPPORTS_INDIRECT_BRANCH_THUNK)
                if(COMPILER_SUPPORTS_INDIRECT_BRANCH_THUNK)
                    target_compile_options(${_target} PRIVATE -mindirect-branch=thunk)
                endif()
                check_c_compiler_flag(-mfunction-return=thunk COMPILER_SUPPORTS_FUNCTION_RETURN_THUNK)
                if(COMPILER_SUPPORTS_FUNCTION_RETURN_THUNK)
                    target_compile_options(${_target} PRIVATE -mfunction-return=thunk)
                endif()
                check_c_compiler_flag(-mindirect-branch-register COMPILER_SUPPORTS_INDIRECT_BRANCH_REGISTER)
                if(COMPILER_SUPPORTS_INDIRECT_BRANCH_REGISTER)
                    target_compile_options(${_target} PRIVATE -mindirect-branch-register)
                endif()
            endif()
        endif()
    endif()
endmacro()

macro(target_compile_warnings _target)
    if(HAS_DEVEL_FILE)
        # "MSVC" is also set for Microsoft's compiler with a Clang front end and their code generator ("Clang/C2"), so 
        # we check for clang and treat that differently.
        if(MSVC AND NOT CLANG)
            # Enable all warnings first
            target_compile_option(${_target} PRIVATE -Wall)

            # Unfortunately, MSVC does not appear to have an equivalent to "__attribute__((unused))" to mark a particular
            # function parameter as being known to be unused, so that the compiler won't warn about it (for example, the
            # function might have that parameter because a pointer to it is being used, and the signature of that function
            # includes that parameter). C++ lets you give a parameter a type but no name, but C doesn't have that.
            target_compile_option(${_target} PRIVATE -wd4100)

            # In theory, we care whether somebody uses f() rather than f(void) to declare a function with no arguments,
            # but, in practice, there are places in the Windows header files that appear to do that, so we squelch that
            # warning.
            target_compile_option(${_target} PRIVATE -wd4255)

            # Windows FD_SET() generates this, so we suppress it.
            target_compile_option(${_target} PRIVATE -wd4548)

            # Perhaps testing something #defined to be 0 with #ifdef is an error, and it should be tested with #if, but
            # perhaps it's not, and Microsoft does that in its headers, so we squelch that warning.
            target_compile_option(${_target} PRIVATE -wd4574)

            # The Windows headers also test not-defined values in #if, so we don't want warnings about that, either.
            target_compile_option(${_target} PRIVATE -wd4668)

            # Disable warnings about whether a function is, or isn't, going to be expanded inline.
            target_compile_option(${_target} PRIVATE -wd4710 -wd4711)

            # Disable warnings about whether we're adding padding bytes after structure members.
            target_compile_option(${_target} PRIVATE -wd4820)

            # Disable warnings about Spectre mitigation
            target_compile_option(${_target} PRIVATE -wd5045)

            # Treat all (remaining) warnings as errors.
            target_compile_option(${_target} PRIVATE -WX)
        else()
            # Other compilers, including MSVC with a Clang front end and Microsoft's code generator.  We currently treat
            # them as if they all support GCC-style -W options.
            target_compile_option(${_target} PRIVATE -Wall)
            target_compile_option(${_target} PRIVATE -Wcomma)
            # The concept of covered switch default is not safe in C because enums are not strongly-typed.
            target_compile_option(${_target} PRIVATE -Wno-covered-switch-default)
            target_compile_option(${_target} PRIVATE -Wdocumentation)
            target_compile_option(${_target} PRIVATE -Wformat-nonliteral)
            target_compile_option(${_target} PRIVATE -Wmissing-noreturn)
            target_compile_option(${_target} PRIVATE -Wmissing-prototypes)
            target_compile_option(${_target} PRIVATE -Wmissing-variable-declarations)
            target_compile_option(${_target} PRIVATE -Wpointer-arith)
            target_compile_option(${_target} PRIVATE -Wpointer-sign)
            target_compile_option(${_target} PRIVATE -Wshadow)
            target_compile_option(${_target} PRIVATE -Wsign-compare)
            target_compile_option(${_target} PRIVATE -Wshorten-64-to-32)
            target_compile_option(${_target} PRIVATE -Wstrict-prototypes)
            target_compile_option(${_target} PRIVATE -Wunreachable-code)
            target_compile_option(${_target} PRIVATE -Wunused-parameter)
            target_compile_option(${_target} PRIVATE -Wused-but-marked-unused)
        endif()
    endif()
endmacro()

########################################################################################################################
#   Configuration
########################################################################################################################

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckCCompilerFlag)
include(CheckFunctionExists)
include(CMakePushCheckState)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckTypeSize)
include(CheckLibraryExists)

# Find .devel file used to enable extra compiler warnings.
message(CHECK_START "Checking .devel file")
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.devel)
    set(DEVEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.devel)
    set(HAS_DEVEL_FILE YES)
elseif(EXISTS ${CMAKE_BINARY_DIR}/.devel)
    set(DEVEL_FILE ${CMAKE_BINARY_DIR}/.devel)
    set(HAS_DEVEL_FILE YES)
endif()
if (HAS_DEVEL_FILE)
    message(CHECK_PASS "found ${${PROJECT_NAME_UPPERCASE}_DEVEL_FILE}")
else()
    message(CHECK_FAIL "not found")
endif()

check_stdc_headers()
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stdbool.h HAVE_FLOAT_H)
check_include_file(stdbool.h HAVE_SETJMP_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)

# Tests are a bit expensive with Visual Studio, so we skip tests for UN*X-only headers and functions on Windows.
if(NOT WIN32)
    check_include_file(unistd.h HAVE_UNISTD_H)
    check_include_file(bitypes.h HAVE_SYS_BITYPES_H)
    check_include_file(sys/stat.h HAVE_SYS_STAT_H)
    check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
    check_include_file(sys/select.h HAVE_SYS_SELECT_H)
    check_include_file(sys/ioccom.h HAVE_SYS_IOCCOM_H)
    check_include_file(sys/sockio.h HAVE_SYS_SOCKIO_H)
endif()

# Check for __atomic_load_n() and __atomic_store_n() builtins.
# We can't use check_function_exists(), as it tries to declare the function, and attempting to declare a compiler
# builtin can produce an error. We don't use check_symbol_exists() as it expects a header file to be specified to
# declare the function, but there isn't such a header file. Hence we use check_c_source_compiles().
check_c_source_compiles("int main(void) { int i = 17; return __atomic_load_n(&i, __ATOMIC_RELAXED); }"
                        HAVE___ATOMIC_LOAD_N)
check_c_source_compiles("int main(void) { int i; __atomic_store_n(&i, 17, __ATOMIC_RELAXED); return 0; }"
                        HAVE___ATOMIC_STORE_N)

# Check for safe strerror functions defined in C11, GNU and POSIX.
check_function_exists(strerror_s HAVE_STRERROR_S)
check_function_exists(strerror_r HAVE_STRERROR_R)
if(HAVE_STRERROR_R)
    # We have strerror_r; if we define _GNU_SOURCE, is it a POSIX-compliant strerror_r() or a GNU strerror_r()?
    # On Linux an XSI-compliant version of strerror_r() is provided only if 
    # (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
    # On macOS 12 (El Capitan) and up strerror_r si POSIX-compliant.
    check_c_source_compiles(${CMAKE_HELPER_PATH}/check_strerror_r.c HAVE_GNU_STRERROR_R)
    if(NOT HAVE_GNU_STRERROR_R)
        set(HAVE_POSIX_STRERROR_R YES)
    endif(NOT HAVE_GNU_STRERROR_R)
endif()

# Make sure we have vsnprintf() and snprintf(); we require them. We use check_symbol_exists(), as they aren't
# necessarily external functions - in Visual Studio, for example, they're inline functions calling a common external
# function.
check_symbol_exists(vsnprintf "stdio.h" HAVE_VSNPRINTF)
if(NOT HAVE_VSNPRINTF)
    message(FATAL_ERROR "vsnprintf() is required but was not found")
endif()
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)
if(NOT HAVE_SNPRINTF)
    message(FATAL_ERROR "snprintf() is required but was not found")
endif()

# Check GNU string functions to which we have fallbacks
check_c_source_compiles(${CMAKE_HELPER_PATH}/check_asprintf.c HAVE_GNU_ASPRINTF)
check_c_source_compiles(${CMAKE_HELPER_PATH}/check_vasprintf.c HAVE_GNU_VASPRINTF)
# Check string functions to which we have fallbacks
check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strtok_r HAVE_STRTOK_R)
# Check Un*x only functions
if(NOT WIN32)
    check_function_exists(vsyslog HAVE_VSYSLOG)
    check_function_exists("fcntl" HAVE_FCNTL)
    check_function_exists("poll" HAVE_POLL)
endif()

# Store libraries discovered by the tests.
set(PRIVATE_LINK_LIBRARIES "")

# We require getaddrinfo(), inet_ntop() and inet_pton(). On UN*X systems, we also prefer versions of recvmsg() that
# conform to the Single UNIX Specification, so that we can check whether a datagram received with recvmsg() was
# truncated when received due to the buffer being too small. On Windows, getaddrinfo(), inet_pton() and inet_ntop() are
# in the ws2_32 library. On most UN*X systems, they're available in the system library.
#
# Under Solaris, we need to link with libsocket and libnsl to get getaddrinfo() and getnameinfo() and, if we have
# libxnet, we need to link with libxnet before libsocket to get a version of recvmsg() that conforms to the Single
# UNIX Specification.
#
# We use getaddrinfo() because we want a portable thread-safe way of getting information for a host name or port; there
# exist _r versions of gethostbyname() and getservbyname() on some platforms, but not on all platforms.
#
# NOTE: if you hand check_library_exists a variable that's been set as the last argument, it skips the test, so we need
# different variables.
if(WIN32)
    # We need winsock2.h and ws2tcpip.h
    cmake_push_check_state()
        set(CMAKE_REQUIRED_LIBRARIES ws2_32)
        check_symbol_exists(getaddrinfo "winsock2.h;ws2tcpip.h" LIBWS2_32_HAS_GETADDRINFO)
        check_symbol_exists(inet_pton "winsock2.h;ws2tcpip.h" LIBWS2_32_HAS_INET_PTON)
        check_symbol_exists(inet_ntop "winsock2.h;ws2tcpip.h" LIBWS2_32_HAS_INET_NTOP)
    cmake_pop_check_state()
    if(LIBWS2_32_HAS_GETADDRINFO AND LIBWS2_32_HAS_INET_PTON AND LIBWS2_32_HAS_INET_NTOP)
        set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} ws2_32)
    elseif(NOT LIBWS2_32_HAS_GETADDRINFO)
        message(FATAL_ERROR "getaddrinfo is required, but was not found")
    elseif(NOT LIBWS2_32_HAS_INET_PTON)
        message(FATAL_ERROR "inet_pton is required, but was not found")
    elseif(NOT LIBWS2_32_HAS_INET_NTOP)
        message(FATAL_ERROR "inet_ntop is required, but was not found")
    endif()
else(WIN32)
    # On UN*X first try the system libraries, then try the libraries for Solaris and possibly other systems that picked
    # up the System V library split.
    check_function_exists(getaddrinfo STDLIBS_HAVE_GETADDRINFO)
    if(NOT STDLIBS_HAVE_GETADDRINFO)
        # Not found in the standard system libraries. Try libsocket, which requires libnsl.
        cmake_push_check_state()
            set(CMAKE_REQUIRED_LIBRARIES nsl)
            check_library_exists(socket getaddrinfo "" LIBSOCKET_HAS_GETADDRINFO)
        cmake_pop_check_state()
        if(LIBSOCKET_HAS_GETADDRINFO)
            # OK, we found it in libsocket.
            set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} socket nsl)
        else()
            check_library_exists(network getaddrinfo "" LIBNETWORK_HAS_GETADDRINFO)
            if(LIBNETWORK_HAS_GETADDRINFO)
                # OK, we found it in libnetwork (Haiku).
                set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} network)
            else()
                # We didn't find anything.
                message(FATAL_ERROR "getaddrinfo is required, but was not found")
            endif()
        endif()

        # OK, do we have recvmsg() in libxnet?
        # We also link with libsocket and libnsl.
        cmake_push_check_state()
            set(CMAKE_REQUIRED_LIBRARIES socket nsl)
            check_library_exists(xnet recvmsg "" LIBXNET_HAS_RECVMSG)
        cmake_pop_check_state()
        if(LIBXNET_HAS_RECVMSG)
            # Yes - link with it as well.
            set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} xnet)
        endif()
    endif()

    # Requires inet_pton
    check_function_exists("inet_pton" HAS_INET_PTON)
    if (NOT HAS_INET_PTON)
        message(FATAL_ERROR "inet_pton is required, but was not found")
    endif()

    # Requires inet_ntop
    check_function_exists("inet_ntop" HAS_INET_NTOP)
    if (NOT HAS_INET_NTOP)
        message(FATAL_ERROR "inet_ntop is required, but was not found")
    endif()
endif(WIN32)

# Check for reentrant versions of getnetbyname_r() as provided by Linux (glibc).
# Only do the check if we have a declaration of getnetbyname_r(); without it, we can't check which API it has.
# (We assume that if there's a declaration, it has a prototype, so that the API can be checked.)
if(NOT WIN32)
    cmake_push_check_state()
        set(CMAKE_REQUIRED_LIBRARIES ${PRIVATE_LINK_LIBRARIES})
        check_symbol_exists(getnetbyname_r netdb.h NETDB_H_DECLARES_GETNETBYNAME_R)
        if(NETDB_H_DECLARES_GETNETBYNAME_R)
            check_c_source_compiles(${CMAKE_HELPER_PATH}/check_getnetbyname_r.c HAVE_GETNETBYNAME_R)
        endif()
    cmake_pop_check_state()
endif(NOT WIN32)

# Check for time functions.
if(WIN32)
    # Check for timeGetTime on winmm. It is not affected by ACPI and process migration between multiple CPUs like
    # QueryPerformanceCounter. It can have a better resolution than GetTickCount (up to 1ms) and it is not affected by
    # system time changes.
    cmake_push_check_state()
        set(CMAKE_REQUIRED_LIBRARIES winmm)
        check_symbol_exists(timeGetTime "windows.h;timeapi.h" LIBWINMM_HAS_TIMEGETTIME)
    cmake_pop_check_state()
    if(LIBWINMM_HAS_TIMEGETTIME)
        set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} winmm)
    else()
        message(FATAL_ERROR "timeGetTime is required, but was not found")
    endif()
else()
    # Check for clock_gettime in librt. Note that Linux/Android behaviour is different than iOS (and possibly macOS).
    # To get elapsed realtime (including any time that the system is suspended)
    #   iOS : CLOCK_MONOTONIC
    #   Linux/Android : CLOCK_BOOTTIME
    # To get uptime (does not count time that the system is suspended)
    #   iOS : CLOCK_UPTIME_RAW
    #   Linux/Android : CLOCK_MONOTONIC
    cmake_push_check_state()
        set(CMAKE_REQUIRED_LIBRARIES rt)
        check_symbol_exists(clock_gettime "time.h" LIBRT_HAS_CLOCK_GETTIME)
        if (LIBRT_HAS_CLOCK_GETTIME)
            set(HAVE_CLOCK_GETTIME TRUE)
            set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} rt)
        endif()
    cmake_pop_check_state()
endif()

# Check for sockaddr_storage and socklen_t.
# There's no check_type() macro that it only checks for the existence of the structure type so we use check_type_size()
# and ignore the size.
cmake_push_check_state()
    if(WIN32)
        set(CMAKE_EXTRA_INCLUDE_FILES ws2tcpip.h winsock2.h)
    else()
        set(CMAKE_EXTRA_INCLUDE_FILES unistd.h sys/socket.h)
    endif()
    check_type_size("struct sockaddr_storage" HAVE_STRUCT_SOCKADDR_STORAGE)
    check_type_size("socklen_t" HAVE_SOCKLEN_T)
cmake_pop_check_state()

# Check if sockaddr has sa_len.
if(WIN32)
    check_struct_has_member("struct sockaddr" sa_len winsock2.h HAVE_STRUCT_SOCKADDR_SA_LEN)
else()
    check_struct_has_member("struct sockaddr" sa_len sys/socket.h HAVE_STRUCT_SOCKADDR_SA_LEN)
endif()

# Check for IPv6 support by checking if sockaddr_in6 exists and has sin6_addr.
if(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6)
    if(WIN32)
        check_struct_has_member("struct sockaddr_in6" sin6_addr ws2tcpip.h
                          HAVE_SOCKADDR_IN6_SIN6_ADDR)
        check_struct_has_member("struct sockaddr_in6" sin6_scope_id ws2tcpip.h
                          HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID)
    else()
        check_struct_has_member("struct sockaddr_in6" sin6_addr "sys/socket.h;netinet/in.h"
                          HAVE_SOCKADDR_IN6_SIN6_ADDR)
        check_struct_has_member("struct sockaddr_in6" sin6_scope_id "sys/socket.h;netinet/in.h"
                          HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID)
    endif()
    if(NOT HAVE_SOCKADDR_IN6_SIN6_ADDR)
        message(STATUS "struct sockaddr_in6 not available, disabling IPv6 support")
        set(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6 OFF CACHE BOOL "Enable IPv6" FORCE)
        set(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6DUAL OFF CACHE BOOL "Enable IPv6 Dual Stack" FORCE)
    endif()
endif()

if(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6 AND if(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6DUAL))
    if(NOT WIN32)
        check_c_source_compiles(${CMAKE_HELPER_PATH}/check_ipv6_dualstack.c HAVE_IPV6DUAL)
    endif()
    if (NOT HAVE_IPV6DUAL)
        message(STATUS "IPv6 Dual Stack is not available, disabling IPv6 Dual Stack support")
        set(${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6DUAL OFF CACHE BOOL "Enable IPv6 Dual Stack" FORCE)
    endif()
endif()

# Check if we have ffs() and if it is declared in <strings.h>
check_function_exists(ffs HAVE_FFS)
if(HAVE_FFS)
# OK, we have ffs(). Is it declared in <strings.h>? This test fails if we don't have <strings.h> or if we do
# but it doesn't declare ffs().
check_symbol_exists(ffs strings.h STRINGS_H_DECLARES_FFS)
endif()

# Check Pthreads.
# We might need it, because some libraries we use might use them, but we don't necessarily depend on them.
# That's only on UN*X; on Windows, if they use threads, we assume they're native Windows threads.
if(NOT WIN32)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  find_package(Threads)
  if(NOT CMAKE_USE_PTHREADS_INIT)
    # If it's not pthreads, we won't use it; we use it for libraries that require it.
    set(CMAKE_THREAD_LIBS_INIT "")
  endif(NOT CMAKE_USE_PTHREADS_INIT)
endif(NOT WIN32)


########################################################################################################################
#   Targets
########################################################################################################################

# Add library without a specific STATIC/SHARED because cmake uses the value of BUILD_SHARED_LIBS to determine whether
# it should one or the other.
add_library(${PROJECT_NAME})

# It's useful to declare an alias so super-projects can link to our library by using namespaces regardless of adding the
# targets by using find_package() or add_subdirectory()
add_library(${PROJECT_NAME_CAPITALIZED}::${PROJECT_NAME_CAPITALIZED} ALIAS ${PROJECT_NAME})

# Our headers are located in two different places: inside src/ (private prototypes and definitions) and in include/,
# where our public headers live. For building our library we need all headers in both locations, so INCLUDE_DIRS must
# contain src/, as well as include/. On the other hand, users of clarinet only need to know about the location of the
# public headers, so INTERFACE_INCLUDE_DIRS only needs to contain include/, but not src/. There is still a problem,
# though. While building clarinet, include/ is at /project/dir/include/, but after installing our library, it will be
# under ${CMAKE_INSTALL_PREFIX}/include/. Therefore, the location of this directory needs to be different depending on
# whether we are building or installing the library. To solve this problem, we use generator expressions, which set the
# correct path depending on the situation.
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/CMakeConfig
)

target_sources(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/${PROJECT_NAME}.h>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}/${PROJECT_NAME}.h>
    PRIVATE
        src/portability.h
        src/${PROJECT_NAME}.c
        src/protocols/dtlc.c
        src/protocols/dtls.c
        src/protocols/udtp.c
        src/protocols/udtps.c
        src/protocols/enet.c
        src/protocols/enets.c
        src/protocols/tls.c
)

# Add system family specific sources, mostly fallbacks.
if(WIN32)
    # Add common files to all WIN32 platforms (WINDOWS, UWP, XBOXONE...)
    target_sources(${PROJECT_NAME}
        PRIVATE
            src/platforms/windows/fallback/asprintf.c
            src/platforms/windows/fallback/getopt.c
    )
elseif(UNIX)
    # Add common files to all UNIX platforms which includes (LINUX, ANDROID, MACOS, IOS, TVOS, WATCHOS, PS4, ...)
    target_sources(${PROJECT_NAME}
        PRIVATE
            src/platforms/unix/fallback/asprintf.c
            src/platforms/unix/fallback/getopt.c
            src/platforms/unix/fallback/strlcat.c
            src/platforms/unix/fallback/strlcpy.c
            src/platforms/unix/fallback/strtok_r.c
    )
else()
    message(FATAL_ERROR "Cannot determine family for system '${CMAKE_SYSTEM_NAME}'")
endif()

# Add system specific sources. 
# Each platform must have specific UDP and TCP implementations which all other protocols will be based on.
target_sources(${PROJECT_NAME}
        PRIVATE
            src/platforms/${PROJECT_SYSTEM_NAME}/addr.c
            src/platforms/${PROJECT_SYSTEM_NAME}/udp.c
            src/platforms/${PROJECT_SYSTEM_NAME}/tcp.c
)

# Set project version for platforms add that information to the binary
# Hide symbols and inline functions (i.e. -fvisibility=hidden and -fvisibility-inlines-hidden)
# Set up public header
# Enable position independent code (i.e. -fPIC) if building shared lib.
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    PUBLIC_HEADER include/${PROJECT_NAME}/${PROJECT_NAME}.h
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

# Add DLL resource file on Windows
if (WIN32)
    if (BUILD_SHARED_LIBS)
        target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCE_DIR}/src/${PROJECT_NAME}.rc)
    endif()
endif()

# Set output name
if(WIN32)
    if(MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            COMPILE_PDB_NAME ${PROJECT_NAME}
            OUTPUT_NAME "${PROJECT_NAME}"
        )
    elseif(MINGW)
        # For compatibility, build the shared library without the "lib" prefix on MinGW as well.
        set_target_properties(${PROJECT_NAME} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${PROJECT_NAME}"
        )
    endif()
else() # UN*X
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
    )
    if(BUILD_SHARED_LIBS)
        if(APPLE)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                SOVERSION A
            )
        else()
            set_target_properties(${PROJECT_NAME} PROPERTIES
                SOVERSION ${PROJECT_VERSION_MAJOR}
            )
        endif()
    endif(BUILD_SHARED_LIBS)
endif()

# Export a symbol to indicate that we're building the library, rather than a program using the library, and thus that
# we're supposed to export symbols defined in our public header files.
if (BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            ${PROJECT_NAME_UPPERCASE}_EXPORT
    )
else()
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            ${PROJECT_NAME_UPPERCASE}_STATIC
    )
endif()

# Main target requires C99
target_compile_features(${PROJECT_NAME}
    PUBLIC
        c_std_99
        c_function_prototypes
        c_restrict
    PRIVATE
        c_variadic_macros
)

# Link private libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PRIVATE_LINK_LIBRARIES}
)

# Treat source files as being in UTF-8 with MSVC if it's not using the Clang front end. We assume that UTF-8 source is
# OK with other compilers and with MSVC if it's using the Clang front end.
if(MSVC AND NOT ${CMAKE_C_COMPILER} MATCHES "clang*")
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# If we're building with MinGW, we need to specify _WIN32_WINNT as 0x0600 ("NT 6.0", a/k/a Vista/Windows Server 2008)
# or higher in order to get the full IPv6 API, including inet_ntop(), and we need to specify it as 0x0601 ("NT 6.1",
# a.k.a Windows 7) or higher in order to get NdisMediumIP.
#
# NOTE: clarinet does *NOT* work with msvcrt.dll; it must link with a newer version of the C library,
# i.e. Visual Studio 2015 or later, as it depends on C99 features introduced in VS 2015.
if(MINGW)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            _WIN32_WINNT=0x0601)
endif()

# Report target library type
if (BUILD_SHARED_LIBS)
    message(STATUS "Target library type is SHARED")
else()
    message(STATUS "Target library type is STATIC")
endif()

# Setup profiler instrumentation
message(STATUS "Profiler instrumentation is ${${PROJECT_NAME_UPPERCASE}_ENABLE_PROFILER}")
target_profiler(${PROJECT_NAME})

# Setup system runtime linkage
message(STATUS "Static runtime is ${${PROJECT_NAME_UPPERCASE}_USE_STATIC_RT}")
if(BUILD_SHARED_LIBS)
    target_static_runtime(${PROJECT_NAME})
else()
    message(STATUS "Ignoring option ${PROJECT_NAME_UPPERCASE}_USE_STATIC_RT for target ${PROJECT_NAME} (irrelevant for static libraries).")
endif ()

message(STATUS "Stack protection is ${${PROJECT_NAME_UPPERCASE}_USE_STACK_PROTECTION}")
target_stack_protection(${PROJECT_NAME})

message(STATUS "Spectre mitigation is ${${PROJECT_NAME_UPPERCASE}_USE_SPECTRE_MITIGATION}")
target_spectre_mitigation(${PROJECT_NAME})

target_compile_warnings(${PROJECT_NAME})

# TODO: Include support for sanitizers on debug builds
# See: Sanitizer options libpcap/CMakeLists.txtx
# See: https://docs.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-160
# See: https://devblogs.microsoft.com/cppblog/address-sanitizer-for-msvc-now-generally-available/

########################################################################################################################
#   Submodules
########################################################################################################################

# Include support to add git submodules
include(AddGitSubmodule)

# MbedTLS
if(${PROJECT_NAME_UPPERCASE}_ENABLE_TLS OR ${PROJECT_NAME_UPPERCASE}_ENABLE_DTLS)
    message(CHECK_START "Adding MbedTLS")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    set(MBEDTLS_PROJECT_NAME "mbed TLS")
    set(CMAKE_PROJECT_${MBEDTLS_PROJECT_NAME}_INCLUDE_BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MbedTLSIncludeBefore.cmake)
    # Use MbedTLS static libs only
    set(USE_STATIC_MBEDTLS_LIBRARY ON) # CACHE BOOL "Build mbed TLS static library." FORCE)
    set(USE_SHARED_MBEDTLS_LIBRARY OFF) # CACHE BOOL "Build mbed TLS shared library." FORCE)
    # Disable MbedTLS programs and testing
    set(ENABLE_PROGRAMS OFF) # CACHE BOOL "Build mbed TLS programs." FORCE)
    set(ENABLE_TESTING OFF) # CACHE BOOL "Build mbed TLS tests." FORCE)
    set(LINK_WITH_PTHREAD OFF) # CACHE BOOL "Explicitly link mbed TLS library to pthread." FORCE)
    set(LINK_WITH_TRUSTED_STORAGE OFF) # CACHE BOOL "Explicitly link mbed TLS library to trusted_storage." FORCE)
    # Add submodule
    add_git_submodule(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls)
    set(HAVE_MBEDTLS YES)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            mbedtls
            mbedx509
            mbedcrypto
    )
    list(POP_BACK CMAKE_MESSAGE_INDENT)
    message(CHECK_PASS "done")
endif()

# On macOS, build target for the appropriate architectures, if CMAKE_OSX_ARCHITECTURES isn't set (if it is, let that
# control the architectures for which to build it).
if(APPLE AND "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    # Get the major version of Darwin.
    string(REGEX MATCH "^([0-9]+)" SYSTEM_VERSION_MAJOR "${CMAKE_SYSTEM_VERSION}")

    if(SYSTEM_VERSION_MAJOR LESS 8)
        # Pre-Tiger.  Build only for 32-bit PowerPC.
        set(OSX_LIBRARY_ARCHITECTURES "ppc")
    elseif(SYSTEM_VERSION_MAJOR EQUAL 8)
        # Tiger.  Is this prior to, or with, Intel support?
        # Get the minor version of Darwin.
        string(REPLACE "${SYSTEM_VERSION_MAJOR}." "" SYSTEM_MINOR_AND_PATCH_VERSION ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+)" SYSTEM_VERSION_MINOR "${SYSTEM_MINOR_AND_PATCH_VERSION}")
        if(SYSTEM_VERSION_MINOR LESS 4)
            # Prior to Intel support.  Build for 32-bit PowerPC and 64-bit PowerPC, with 32-bit PowerPC first.  
            # (I'm guessing that's what Apple does.)
            set(OSX_LIBRARY_ARCHITECTURES "ppc;ppc64")
        elseif(SYSTEM_VERSION_MINOR LESS 7)
            # With Intel support but prior to x86-64 support. Build for 32-bit PowerPC, 64-bit PowerPC, and 32-bit x86,
            # with 32-bit PowerPC first. (I'm guessing that's what Apple does.)
            set(OSX_LIBRARY_ARCHITECTURES "ppc;ppc64;i386")
        else()
            # With Intel support including x86-64 support. Build for 32-bit PowerPC, 64-bit PowerPC, 32-bit x86,
            # and x86-64, with 32-bit PowerPC first. (I'm guessing that's what Apple does.)
            set(OSX_LIBRARY_ARCHITECTURES "ppc;ppc64;i386;x86_64")
        endif()
    elseif(SYSTEM_VERSION_MAJOR EQUAL 9)
        # Leopard.  Build for 32-bit PowerPC, 64-bit PowerPC, 32-bit x86, and x86-64, with 32-bit PowerPC first. 
        # (That's what Apple does.)
        set(OSX_LIBRARY_ARCHITECTURES "ppc;ppc64;i386;x86_64")
    elseif(SYSTEM_VERSION_MAJOR EQUAL 10)
        # Snow Leopard.  Build for x86-64, 32-bit x86, and 32-bit PowerPC, with x86-64 first.  (That's what Apple does, 
        # even though Snow Leopard doesn't run on PPC, so PPC libclarinet runs under Rosetta, and Rosetta doesn't 
        # support BPF ioctls, so PPC programs can't do live captures.)
        set(OSX_LIBRARY_ARCHITECTURES "x86_64;i386;ppc")
    else()
        # Post-Snow Leopard.  Build for x86-64 and 32-bit x86, with x86-64 first. (That's what Apple does) 
        # Update if and when Apple drops support for 32-bit x86 code and if and when Apple adds ARM-based Macs. 
        # TODO: what about iOS, tvOS, watchOS ?
        # Check whether we're building with MbedTLS. If so, don't bother trying to build fat.
        if(HAVE_MBEDTLS)
          set(X86_32_BIT_SUPPORTED NO)
          set(OSX_LIBRARY_ARCHITECTURES "x86_64")
          message(STATUS "Assuming MbedTLS libraries are 64-bit only therefore not compiling for 32-bit x86")
        else(HAVE_MBEDTLS)
          # Now, check whether we *can* build for i386.
          cmake_push_check_state()
          set(CMAKE_REQUIRED_FLAGS "-arch i386")
          check_c_source_compiles("int main(void) { return 0; }" X86_32_BIT_SUPPORTED)
          cmake_pop_check_state()
          if(X86_32_BIT_SUPPORTED)
              set(OSX_LIBRARY_ARCHITECTURES "x86_64;i386")
          else(X86_32_BIT_SUPPORTED)
              set(OSX_LIBRARY_ARCHITECTURES "x86_64")
              # We can't build fat; suggest that the user install the /usr/include headers if they want to build fat.
              if(SYSTEM_VERSION_MAJOR LESS 18)
                  # Pre-Mojave; the command-line tools should be sufficient to enable 32-bit x86 builds.
                  message(STATUS "Compiling for 32-bit x86 gives an error; try installing the command-line tools")
              else()
                  message(STATUS "Compiling for 32-bit x86 gives an error; try installing the command-line tools and, after that, installing the /usr/include headers from the /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg package")
              endif()
          endif(X86_32_BIT_SUPPORTED)
        endif(HAVE_MBEDTLS)
    endif()
    set_target_properties(${LIBRARY_NAME} PROPERTIES OSX_ARCHITECTURES "${OSX_LIBRARY_ARCHITECTURES}")
endif()

# Extra compiler options for targets to request -Werror or its equivalent if required.
# The CMake variable name cannot be CFLAGS because that is already used for a different purpose in CMake.
# usage: cmake -DEXTRA_CFLAGS='-Wall -Wextra -Werror' ...
if(NOT "${EXTRA_CFLAGS}" STREQUAL "")
    foreach(_extra_cflag ${EXTRA_CFLAGS})
        target_compile_option(${PROJECT_NAME} PRIVATE "${_extra_cflag}")
    endforeach(_extra_cflag)
    message(STATUS "Extra compile options: (${EXTRA_CFLAGS})")
endif()

# Save configuration
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/CMakeConfig/config.h)

########################################################################################################################
# Report Features
########################################################################################################################

message(STATUS "Configured features")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

message(STATUS "Profiler is ${${PROJECT_NAME_UPPERCASE}_ENABLE_PROFILER}")
message(STATUS "Log is ${${PROJECT_NAME_UPPERCASE}_ENABLE_LOG}")
message(STATUS "IPv6 is ${${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6}")
message(STATUS "IPv6 Dual Stack is ${${PROJECT_NAME_UPPERCASE}_ENABLE_IPV6DUAL}")
message(STATUS "UDP is ${${PROJECT_NAME_UPPERCASE}_ENABLE_UDP}")
message(STATUS "DTLC is ${${PROJECT_NAME_UPPERCASE}_ENABLE_DTLC}")
message(STATUS "DTLS is ${${PROJECT_NAME_UPPERCASE}_ENABLE_DTLS}")
message(STATUS "UDTP is ${${PROJECT_NAME_UPPERCASE}_ENABLE_UDTP}")
message(STATUS "UDTPS is ${${PROJECT_NAME_UPPERCASE}_ENABLE_UDTPS}")
message(STATUS "ENET is ${${PROJECT_NAME_UPPERCASE}_ENABLE_ENET}")
message(STATUS "ENETS is ${${PROJECT_NAME_UPPERCASE}_ENABLE_ENETS}")
message(STATUS "TCP is ${${PROJECT_NAME_UPPERCASE}_ENABLE_TCP}")
message(STATUS "TLS is ${${PROJECT_NAME_UPPERCASE}_ENABLE_TLS}")
list(POP_BACK CMAKE_MESSAGE_INDENT)


########################################################################################################################
# Install
########################################################################################################################

# Include GNU standard installation directories, which actually are also defined, to some degree, by autotools, and at
# least some of which are general UN*X conventions.
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR cmake)

# This is required so that the exported target has the name PROJECT_NAME_CAPITALIZED and not PROJECT_NAME
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME ${PROJECT_NAME_CAPITALIZED})

# Install project
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    # For DLL platforms (all Windows-based systems including Cygwin), the DLL is treated as RUNTIME and the import
    # library is treated as an ARCHIVE target although it's not a static lib really.
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# We need to install static libraries manually when configured to produce a static lib while taking care to NOT include
# header files (or cmake files). Static library dependencies are not *automagically* installed and with EXCLUDE_FROM_ALL
# the targets in each subdir (git submodule) are not installed by the ALL target.
# NOTE: Since Cmake 3.13 install(TARGETS) can install targets that were created in other directories. When using such
# cross-directory install rules, running make install (or similar) from a subdirectory will not guarantee that targets
# from other directories are up-to-date.
if (NOT BUILD_SHARED_LIBS)
    # Install MbedTLS static libs
    if(HAVE_MBEDTLS)
        install(TARGETS mbedtls mbedx509 mbedcrypto
            EXPORT ${PROJECT_NAME}-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
endif()

# Export the targets to a script
install(EXPORT ${PROJECT_NAME}-targets
    FILE
        ${PROJECT_NAME_CAPITALIZED}Targets.cmake
    NAMESPACE
        ${PROJECT_NAME_CAPITALIZED}::
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_CAPITALIZED}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Create Config file
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/templates/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_CAPITALIZED}Config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

# Create pkg-config file
set (PKG_CONFIG_STATIC_DEFINE "")
if (NOT BUILD_SHARED_LIBS)
    set (PKG_CONFIG_STATIC_DEFINE "-D${PROJECT_NAME_UPPERCASE}_STATIC")
endif()
set(PKG_CONFIG_LINK_LIBRARIES "")
get_target_property(PKG_CONFIG_LINK_LIBRARIES ${PROJECT_NAME} INTERFACE_LINK_LIBRARIES)
string(REPLACE "PKG_CONFIG_LINK_LIBRARIES-NOTFOUND" "" PKG_CONFIG_LINK_LIBRARIES ${PKG_CONFIG_LINK_LIBRARIES})
if (NOT PKG_CONFIG_LINK_LIBRARIES STREQUAL "")
    string(REGEX REPLACE "\\$\\<LINK_ONLY:([^>]*)\\>" "-l\\1 " PKG_CONFIG_LINK_LIBRARIES ${PKG_CONFIG_LINK_LIBRARIES})
endif()
string(PREPEND PKG_CONFIG_LINK_LIBRARIES "-l${PROJECT_NAME} ")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

# Install the Config, ConfigVersion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_CAPITALIZED}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_CAPITALIZED}ConfigVersion.cmake
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

# Install pkgconfig file
install(FILES
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)

# Export from the build tree
export(EXPORT ${PROJECT_NAME}-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_CAPITALIZED}Targets.cmake
    NAMESPACE ${PROJECT_NAME_CAPITALIZED}::
)

# Do not export the package by default
cmake_policy(SET CMP0090 NEW)

# Register package in the User Package Registry
export(PACKAGE ${PROJECT_NAME_CAPITALIZED})

########################################################################################################################
#   Tests
########################################################################################################################
include(CTest)
if (${PROJECT_NAME_UPPERCASE}_BUILD_TESTING AND BUILD_TESTING)
    set(HAVE_UNIT_TESTS ON)
else()
    set(HAVE_UNIT_TESTS OFF)
endif()    
message(STATUS "Unit testing is ${HAVE_UNIT_TESTS}")
if(HAVE_UNIT_TESTS)
    add_subdirectory(tests)
endif()

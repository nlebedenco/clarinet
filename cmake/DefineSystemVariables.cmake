# Guard agains multiple includes.
if( _SYSTEM_VARIABLES_DEFINED_ )
    return()
endif()
set(_SYSTEM_VARIABLES_DEFINED_ 1)

# Check known systems 
if (WIN32)
    if (CMAKE_SYSTEM_NAME STREQUAL Windows)
        set(WINDOWS TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
        set(UWP TRUE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Durango)
        set(XBOXONE TRUE)
    endif()
elseif (APPLE)
    if (CMAKE_SYSTEM_NAME STREQUAL tvOS)
        set(TVOS TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL watchOS)
        set(WATCHOS TRUE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin )
        set(MACOS TRUE)
    endif()
else()
    if(CMAKE_SYSTEM_NAME STREQUAL Linux )
        set(LINUX TRUE)
        # In theory, WSL builds should be the same as any other Linux but in practice WSL has many limitations that
        # may affect tests, for example, although WSL supports IPV6 sockets it cannot reach to external IPV6 addresses
        # because of Hyper-V limitations so connect(2) and sendto(2) always fail with ENETUNREACH.
        if (CMAKE_SYSTEM MATCHES "^.*[Mm]icrosoft.*$")
            set(WSL TRUE)
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
        set(FREEBSD TRUE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Orbis)
        set(PS4 TRUE)
    endif()
endif()

# "MSVC" is also set for Microsoft's compiler with a Clang front end and their code generator ("Clang/C2"), so we define
# CLANG is hits case too.
if((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (MSVC AND (CMAKE_C_COMPILER MATCHES "clang*")))
    set(CLANG TRUE)
endif()

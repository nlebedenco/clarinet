########################################################################################################################
#   Configuration
########################################################################################################################

set(${PROJECT_API_PREFIX_UPPERCASE}_TESTS_FOLDER "CTest")


########################################################################################################################
#   Macros
########################################################################################################################

macro(target_test _target)
    add_executable(${_target})
    
    target_sources(${_target}
        PRIVATE 
            src/test.h
            src/test.cpp
    )

    # Set folder
    # Set linker for cmd line param support tests - for now all tests are in CXX this must be reviewed if the test framework changes
    set_target_properties(${_target} PROPERTIES
        FOLDER "${${PROJECT_MACRO_PREFIX}_TESTS_FOLDER}"
        LINKER_LANGUAGE CXX
    )

    target_compile_features(${_target}
        PRIVATE
            cxx_std_11
            cxx_variadic_macros
            cxx_defaulted_functions
            cxx_defaulted_move_initializers
            cxx_nullptr
    )

    # Export a symbol to indicate that we have a config.h auto generated.
    target_compile_definitions(${_target}
        PRIVATE
            HAVE_CONFIG_H=1
    )

    # Export a symbol to indicate that we are building in Microsoft WSL
    if (WSL)
        target_compile_definitions(${_target} PRIVATE __wsl__=1 )
    endif()

    target_profiler(${_target})
    target_static_runtime(${_target})
    target_stack_protection(${_target})
    target_spectre_mitigation(${_target})
    target_compile_warnings(${_target})

    if(MSVC AND NOT CLANG)
        # Disable warning about the optimizer removing inline functions that are not called
        target_compile_option(${_target} PRIVATE -wd4514)    
        # Disable warnings about deleted functions
        target_compile_option(${_target} PRIVATE -wd4623)
        target_compile_option(${_target} PRIVATE -wd4625)
        target_compile_option(${_target} PRIVATE -wd4626)
        target_compile_option(${_target} PRIVATE -wd5026)
        target_compile_option(${_target} PRIVATE -wd5027)
        # Disable warning about the compiler not being able to enforce left-to-right evaluation order in braced initializer list
        target_compile_option(${_target} PRIVATE -wd4868)
        # Disable warning about throwable function exported by C interface in the Win SDK (bad bad microsoft...!)
        target_compile_option(${_target} PRIVATE -wd5039)       
        # Disable warning about unscoped enum        
        target_compile_option(${_target} PRIVATE -wd26812)
        # Disable warning for member variable not initialized
        target_compile_option(${_target} PRIVATE -wd26495)    
        # Disable warning about comparing signed/unsigned int
        target_compile_option(${_target} PRIVATE -wd4388)     
        # Disable warning for abstract/virtual class not having a virtual dtor */
        target_compile_option(${_target} PRIVATE -wd5204)     
    else()
        # Disable warning about non-literal format strings
        target_compile_option(${_target} PRIVATE -Wno-format-nonliteral)
        # Catch2 does implicit convertions that changes sign and we can't avoid it.
        target_compile_option(${_target} PRIVATE -Wno-sign-conversion)
    endif()

    target_include_directories(${_target}
        PRIVATE
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_BINARY_DIR}/CMakeConfig
    )
    
    target_link_libraries(${_target}
        PRIVATE
            ${PROJECT_NAME}
            Catch2::Catch2WithMain
    )

    # Register catch2 tests defined in target
    catch_discover_tests(${_target}
        EXTRA_ARGS -w NoAssertions
        PROPERTIES TIMEOUT 5
    )
endmacro()

########################################################################################################################
#   Tests
########################################################################################################################
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

file(GLOB cmakeTestFiles ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)
list(SORT cmakeTestFiles COMPARE NATURAL CASE INSENSITIVE ORDER ASCENDING)
foreach(cmakeTestFile ${cmakeTestFiles})
    message(STATUS "Including test: ${cmakeTestFile}")
    include(${cmakeTestFile})
endforeach()

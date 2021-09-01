########################################################################################################################
#   Configuration
########################################################################################################################

set(${PROJECT_NAME_UPPERCASE}_TESTS_FOLDER "Tests")


########################################################################################################################
#   Macros
########################################################################################################################

macro(target_test _target)
    add_executable(${_target})

    target_compile_features(${_target}
        PRIVATE
            c_std_99
            c_function_prototypes
            c_restrict
            c_variadic_macros
    )

    # Export a symbol to indicate that we have a config.h auto generated.
    target_compile_definitions(${_target}
        PRIVATE
            HAVE_CONFIG_H
    )
        
    target_profiler(${_target})
    target_static_runtime(${_target})
    target_stack_protection(${_target})
    target_spectre_mitigation(${_target})
    target_compile_warnings(${_target})

    target_include_directories(${_target}
        PRIVATE
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_BINARY_DIR}/CMakeConfig
    )

    target_link_libraries(${_target}
        ${PROJECT_NAME}
        cmocka-static
    )

    set_target_properties(${_target} PROPERTIES
        FOLDER ${${PROJECT_NAME_UPPERCASE}_TESTS_FOLDER}
    )

    add_test(NAME ${_target} COMMAND $<TARGET_FILE:${_target}>)
endmacro()


########################################################################################################################
#   Dependencies
########################################################################################################################

set(CMAKE_PROJECT_cmocka_INCLUDE_BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMockaIncludeBefore.cmake)
include(FetchCMocka)
set_target_properties(cmocka-static PROPERTIES 
    FOLDER ${${PROJECT_NAME_UPPERCASE}_TESTS_FOLDER}/Dependencies
)


########################################################################################################################
#   Tests
########################################################################################################################

file(GLOB cmakeTestFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cmake)
list(SORT cmakeTestFiles COMPARE NATURAL CASE INSENSITIVE ORDER ASCENDING)
foreach(cmakeTestFile ${cmakeTestFiles})
    message("Including ${cmakeTestFile}")
    include(${cmakeTestFile})
endforeach()

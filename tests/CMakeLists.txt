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
            c_variadic_macros
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

target_test(test_details_interface)
target_sources(test_details_interface PRIVATE src/test_details_interface.c)

target_test(test_initialize)
target_sources(test_initialize PRIVATE src/test_initialize.c)

target_test(test_finalize)
target_sources(test_finalize PRIVATE src/test_finalize.c)
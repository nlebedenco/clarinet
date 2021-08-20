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

    target_link_libraries(${_target}
        ${PROJECT_NAME}
        cmocka-static
    )

    add_test(NAME ${_target} COMMAND $<TARGET_FILE:${_target}>)
endmacro()

########################################################################################################################
#   Tests
########################################################################################################################
include(FetchCMocka)

target_test(test_details_interface)
target_sources(test_details_interface
    PRIVATE
        src/test_details_interface.c
)

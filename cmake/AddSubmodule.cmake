cmake_minimum_required(VERSION 3.14)

include(ExternalProject)

function(git_submodule_update dir)
    # add a Git submodule directory to CMake, assuming the
    # Git submodule directory is a CMake project.
    #
    # Usage: in CMakeLists.txt
    #
    # include(AddSubmodule.cmake)
    # add_git_submodule(mysubmod_dir)
    find_package(Git REQUIRED QUIET)
    if(NOT EXISTS ${dir}/.git)
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
endfunction()

function(add_submodule dir)
    git_submodule_update(${dir})
    # Add the subdirectory but exclude targets from the `all` target and from the IDE.
    # Targets can still be referenced though so only those actually used as dependencie will be built and show up in
    # the IDE.
    add_subdirectory(${dir} EXCLUDE_FROM_ALL)
endfunction()

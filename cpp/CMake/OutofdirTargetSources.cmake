include_guard(GLOBAL)

# Before CMake 3.13 it is possible to use target_link_library only on a target that was created in
# the same directory scope, this is why "include" is used instead of "add_subdirectory" in some
# cases. Handling of relative paths has also changed in 3.13, this is why we need the following
# wrapper function.
#
# Idea from: https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
#
# NOTE: This helper function assumes no generator expressions are used
#       for the source files
function(outofdir_target_sources target)

    if(POLICY CMP0076)
        cmake_policy(PUSH)
        cmake_policy(SET CMP0076 OLD)
    endif()

    # Must be using CMake 3.12 or earlier, so simulate the new behavior
    unset(_srcList)
    get_target_property(_targetSourceDir ${target} SOURCE_DIR)

    foreach(src ${ARGN})
        if(src MATCHES "\$\<")
            message(FATAL "Generator expressions not supported: '${arg}'")
        endif()
        if(NOT src MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$" AND
           NOT IS_ABSOLUTE "${src}"
        )
            # Relative path to source, prepend relative to where target was defined
            file(RELATIVE_PATH src "${_targetSourceDir}" "${CMAKE_CURRENT_LIST_DIR}/${src}")
        endif()
        list(APPEND _srcList ${src})
    endforeach()
    target_sources(${target} ${_srcList})

    if(POLICY CMP0076)
        cmake_policy(POP)
    endif()

endfunction()

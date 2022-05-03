# LCOV postprocessing script for sources generated in binary directory

# The current version of coverage data produced by GCC contains relative paths
# for source files in the build directory. For these LCOV with --base-directory
# option prepends that base directory unconditionally and we end up with
# incorrect absolute paths. This script checks if a path exists and if it
# doesn't it replaces the absolute path with the build directory.

# In GCC 8.1 compiling with -fprofile-abs-path should fix the problem.

set(coverage_file ${CMAKE_ARGV3})
set(base_dir ${CMAKE_ARGV4})
set(binary_dir ${CMAKE_ARGV5})

if(NOT coverage_file OR NOT base_dir OR NOT binary_dir)
    message(FATAL_ERROR "Incorrect parameters to script")
endif()

file(STRINGS ${coverage_file} fstrings)
foreach(fstring IN LISTS fstrings)
    string(REGEX MATCH "^SF:(.*)" matches ${fstring})
    if(matches)
        set(path "${CMAKE_MATCH_1}")
        if(NOT EXISTS "${path}")
            message(STATUS "Could not find file: ${path}")
            string(LENGTH "${base_dir}" base_len)
            string(SUBSTRING "${path}" ${base_len} -1 path_rel)

            set(path "${binary_dir}${path_rel}")
            if(NOT EXISTS "${path}")
                message(FATAL_ERROR "File not found: ${path}")
            endif()
        endif()

        # Resolve symlinks
        get_filename_component(path ${path} REALPATH)

        set(fstring "SF:${path}")
    endif()
    set(result "${result}\n${fstring}")
endforeach()
file(WRITE "${coverage_file}" "${result}")

# Check overall code coverage results (from genhtml stdout) against given thresholds

# Example output:
# Overall coverage rate:
#   lines......: 100.0% (10 of 10 lines)
#   functions..: 100.0% (2 of 2 functions)
#   regions....: 100.0% (2 of 2 regions)
#   branches...: 100.0% (2 of 2 branches)

set(coverage_log "${CMAKE_ARGV3}")
set(threshold_line "${CMAKE_ARGV4}")
set(threshold_function "${CMAKE_ARGV5}")
set(threshold_region "${CMAKE_ARGV6}")
set(threshold_branch "${CMAKE_ARGV7}")

if(NOT DEFINED coverage_log OR NOT DEFINED threshold_line OR NOT DEFINED threshold_function
        OR NOT DEFINED threshold_region OR NOT DEFINED threshold_branch)
    message(FATAL_ERROR "Incorrect parameters to script")
endif()

set(coverage_types line function regions branch)
set(coverage_line_pl lines)
set(coverage_function_pl functions)
set(coverage_region_pl regions)
set(coverage_branch_pl branches)

file(STRINGS ${coverage_log} fstrings)
foreach(fstring IN LISTS fstrings)
    foreach(coverage_type IN LISTS coverage_types)
        string(REGEX MATCH "${coverage_${coverage_type}_pl}\\.*: ([0-9]+\\.[0-9]+)% " matches "${fstring}")
        if(matches)
            set(coverage_${coverage_type} "${CMAKE_MATCH_1}")
        endif()
    endforeach()
endforeach()

foreach(coverage_type IN LISTS coverage_types)
    if("${threshold_${coverage_type}}" GREATER 0)
        if(DEFINED "coverage_${coverage_type}")
            if("${coverage_${coverage_type}}" LESS "${threshold_${coverage_type}}")
                set(msg "Coverage violation: ${coverage_type} coverage: ${coverage_${coverage_type}}%,")
                set(msg "${msg} expected: ${threshold_${coverage_type}}%")
                message(SEND_ERROR "${msg}")
            endif()
        else()
           message(SEND_ERROR "Coverage violation: No ${coverage_type} coverage found")
        endif()
    endif()
endforeach()

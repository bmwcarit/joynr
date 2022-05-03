# This CMake script generates an LLVM-COV report
#
# Inputs:
# All llvm-cov profraw files in ${CMAKE_BINARY_DIR}
# All .o files in ${CMAKE_BINARY_DIR}
#
# Parameters:
# BASE_DIR:    The base directory to create coverage for (for example: ${CMAKE_SOURCE_DIR}/src)
# REPORT_DIR:  The directory where reports will be generated in
# REPORT_NAME: File/folder name of the reports
# EXCLUDES:    A list of regular expressions for exclusion

if(NOT DEFINED BASE_DIR OR NOT DEFINED REPORT_DIR)
    message(FATAL_ERROR "Incorrect parameters to script")
endif()

if(NOT REPORT_NAME)
    set(REPORT_NAME "code-coverage-report")
endif()
set(report_path_we "${REPORT_DIR}/${REPORT_NAME}")

# Generate merged profdata file.
set(merged_data_file ${REPORT_DIR}/coverage-merged.profdata)
message(STATUS "Merging LLVM-COV coverage input files")
file(GLOB_RECURSE profraw_files ${CMAKE_BINARY_DIR}/*.profraw)
# We expect LLVM_PROFILE_FILE to be set on test execution.
list(FILTER profraw_files EXCLUDE REGEX "default.profraw")
if(NOT profraw_files)
    message(FATAL_ERROR "No .profraw files found")
endif()
execute_process(
    # "-sparse",  # Leads to smaller .profdata file. But omits files that have 0 coverage.
    COMMAND llvm-profdata merge -o ${merged_data_file} ${profraw_files}
)

# Generate a list of object files.
file(GLOB_RECURSE object_files ${CMAKE_BINARY_DIR}/*.o)
list(FILTER object_files EXCLUDE REGEX ".*CMake.*CompilerId.*")
if(NOT object_files)
    message(FATAL_ERROR "No .o files found")
endif()
list(GET object_files 0 object_file_args)
list(REMOVE_AT object_files 0)
foreach(object_file IN LISTS object_files)
    list(APPEND object_file_args "-object=${object_file}")
endforeach()

# Generate exclude arguments.
foreach(exclude_regex IN LISTS EXCLUDES)
    list(APPEND exclude_regex_args "-ignore-filename-regex=${exclude_regex}")
endforeach()

# Function to log messages without the usual CMake annotations to STDOUT and STDERR.
function(message_raw message stream)
    if(stream STREQUAL STDOUT)
        execute_process(COMMAND ${CMAKE_COMMAND} -E echo "${message}")
    elseif(stream STREQUAL STDERR)
        message("${message}")
    else()
        message(FATAL_ERROR "Incorrect value for parameter stream")
    endif()
endfunction()

# Function to filter out known warnings from log output.
function(print_filtered_errors error_output)
    # 'llvm-cov show -format=html' forcefully enables colorful output.
    string(ASCII 27 esc)
    string(REGEX REPLACE "${esc}\\[.*m" "" error_output "${error_output}")

    # This happens since we list all object files we find. It's safe to ignore to not confuse users.
    string(REGEX REPLACE "warning: [0-9]+ functions have mismatched data\n" "" error_output "${error_output}")

    if(error_output)
        message_raw("${error_output}" STDERR)
    endif()
endfunction()

# Generate HTML report.
message(STATUS "Generating HTML coverage report at file://${report_path_we}/index.html")
execute_process(
    COMMAND llvm-cov show
                     -format=html
                     -output-dir=${report_path_we}
                     -show-line-counts-or-regions
                     -instr-profile=${merged_data_file}
                     -Xdemangler=c++filt
                     ${exclude_regex_args}
                     ${object_file_args}
                     ${BASE_DIR}
                     ${CMAKE_BINARY_DIR}
    ERROR_VARIABLE error_output
)
print_filtered_errors("${error_output}")

# Generate LLVM-COV report (for SonarQube).
message(STATUS "Generating LLVM-COV coverage report at file://${report_path_we}.txt")
execute_process(
    COMMAND llvm-cov show
                     -format=text
                     -instr-profile=${merged_data_file}
                     -Xdemangler=c++filt
                     ${exclude_regex_args}
                     ${object_file_args}
                     ${BASE_DIR}
                     ${CMAKE_BINARY_DIR}
    OUTPUT_FILE ${report_path_we}.txt
    ERROR_VARIABLE error_output
)
print_filtered_errors("${error_output}")

# Generate LCOV report.
message(STATUS "Generating LCOV coverage report at file://${report_path_we}.info")
execute_process(
    COMMAND llvm-cov export
                     -format=lcov
                     -instr-profile=${merged_data_file}
                     -skip-expansions
                     ${exclude_regex_args}
                     ${object_file_args}
                     ${BASE_DIR}
                     ${CMAKE_BINARY_DIR}
    OUTPUT_FILE ${report_path_we}.info
    ERROR_VARIABLE error_output
)
print_filtered_errors("${error_output}")

# Generate coverage summary (for printing it to the console).
message(STATUS "Generating coverage summary")
execute_process(
    COMMAND llvm-cov report
                     -instr-profile=${merged_data_file}
                     ${exclude_regex_args}
                     ${object_file_args}
                     ${BASE_DIR}
                     ${CMAKE_BINARY_DIR}
    OUTPUT_VARIABLE summary_output
    ERROR_VARIABLE error_output
)
print_filtered_errors("${error_output}")

# Reformat the data to print only the totals.
string(REGEX MATCH "\nTOTAL[ \t]+(.*)\n" _ "${summary_output}")
string(REGEX REPLACE "[ \t]+" ";" summary_output "${CMAKE_MATCH_1}")
list(LENGTH summary_output summary_len)
if(NOT summary_len EQUAL 12)
    message(FATAL_ERROR "Failed to interpret coverage summary")
endif()
list(GET summary_output 0 1 2 region_parameters)
list(GET summary_output 3 4 5 function_parameters)
list(GET summary_output 6 7 8 line_parameters)
list(GET summary_output 9 10 11 branch_parameters)

# Function to print the coverage values in a predefined format.
function(print_coverage type total uncovered percent)
    # Generate a padding
    string(LENGTH "${type}" type_len)
    math(EXPR type_len "11 - ${type_len}")
    foreach(i RANGE 1 ${type_len})
        string(APPEND padding ".")
    endforeach()

    # Calculate covered from total and uncovered.
    math(EXPR covered "${total} - ${uncovered}")

    message_raw("${type}${padding}: ${percent} (${covered} of ${total} ${type})" STDOUT)
endfunction()

message_raw("\nOverall coverage rate:" STDOUT)
print_coverage(lines ${line_parameters})
print_coverage(functions ${function_parameters})
print_coverage(regions ${region_parameters})
print_coverage(branches ${branch_parameters})
message_raw("" STDOUT)

# Remove merged profdata file.
file(REMOVE "${merged_data_file}")

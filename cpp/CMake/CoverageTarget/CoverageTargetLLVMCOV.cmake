# Coverage build type
set(CMAKE_CXX_FLAGS_COVERAGE "-g -O0 -fprofile-instr-generate -fcoverage-mapping" CACHE STRING
    "Flags used by the C++ compiler during coverage builds." FORCE
)
set(CMAKE_C_FLAGS_COVERAGE "-g -O0 -fprofile-instr-generate -fcoverage-mapping" CACHE STRING
    "Flags used by the C compiler during coverage builds." FORCE
)
mark_as_advanced(
    CMAKE_CXX_FLAGS_COVERAGE CMAKE_C_FLAGS_COVERAGE
)


find_program(LLVM_COV_EXECUTABLE llvm-cov)

set(LLVM_COV_FIND_REQUIRED TRUE)
find_package_handle_standard_args(LLVM_COV
    REQUIRED_VARS LLVM_COV_EXECUTABLE
)
mark_as_advanced(LLVM_COV_EXECUTABLE)

# LCOV supports exclude patterns, LLVM-COV requires regular expressions.
# In order to support the same interface for excludes, convert the patterns to regular expressions.
foreach(exclude ${_coverage_excludes})
    string(REGEX REPLACE "^\/" "" exclude ${exclude})
    string(REGEX REPLACE "^\\*/" "" exclude ${exclude})
    string(REGEX REPLACE "\\." "\\\\." exclude ${exclude})
    string(REGEX REPLACE "\\*" ".*" exclude ${exclude})
    set(exclude ".*/${exclude}")
    list(APPEND _coverage_excludes_re ${exclude})
endforeach()
# Prepare to pass a list via cmake -P
string(REPLACE ";" "$<SEMICOLON>" _coverage_excludes_re "${_coverage_excludes_re}")

# build_coverage_report target. This target builds the coverage report based on the already generated coverage
# files within the build directory.
add_custom_target(build_coverage_report
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}

    COMMAND ${CMAKE_COMMAND} -DBASE_DIR=${COVERAGE_BASE_DIR}
                             -DREPORT_DIR=${COVERAGE_DIR}
                             -DREPORT_NAME=${PROJECT_NAME}.coverage
                             -DEXCLUDES=${_coverage_excludes_re}
                             -P ${_coverage_script_folder}/ExecuteLLVMCov.cmake
                             | tee ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.log

    COMMAND ${CMAKE_COMMAND} -E echo "Coverage report: file://${COVERAGE_DIR}/index.html"

    COMMENT "Build code coverage report"
    USES_TERMINAL # Ensure ninja outputs to stdout.
    VERBATIM # Properly pass excludes.
)

# coverage target. This target builds the project, executes the tests and builds the report.
add_custom_target(coverage
    COMMAND find ${CMAKE_BINARY_DIR} -name '*.profraw' -delete
    COMMAND rm -f ${CMAKE_BINARY_DIR}/.tests-failed

    # Compile sources first. (It is not possible to depend on a built-in target such as 'all':
    # https://cmake.org/Bug/view.php?id=8438)
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}

    # working escaping for make: \${ARGS} \$\${ARGS}
    # working escaping for ninja: \$\${ARGS}
    # No luck with VERBATIM option.
    COMMAND LLVM_PROFILE_FILE=%1m.profraw ${CMAKE_CTEST_COMMAND} --output-on-failure "\$\${ARGS}" || touch ${CMAKE_BINARY_DIR}/.tests-failed

    # Build the actual report
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target build_coverage_report

    # Check coverage thresholds
    COMMAND ${CMAKE_COMMAND} -P ${_coverage_script_folder}/CheckThresholds.cmake
        ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.log
        ${COVERAGE_THRESHOLD_LINE} ${COVERAGE_THRESHOLD_FUNCTION}
        ${COVERAGE_THRESHOLD_REGION} ${COVERAGE_THRESHOLD_BRANCH}

    # Fail the build if the tests failed.
    COMMAND test ! -f ${CMAKE_BINARY_DIR}/.tests-failed

    COMMENT "Generate code coverage"
    USES_TERMINAL # Ensure ninja outputs to stdout.
)

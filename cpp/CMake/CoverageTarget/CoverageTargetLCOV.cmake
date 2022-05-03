# Coverage build type
set(CMAKE_CXX_FLAGS_COVERAGE "-g -O0 --coverage" CACHE STRING
    "Flags used by the C++ compiler during coverage builds." FORCE
)
set(CMAKE_C_FLAGS_COVERAGE "-g -O0 --coverage" CACHE STRING
    "Flags used by the C compiler during coverage builds." FORCE
)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING
    "Flags used for linking binaries during coverage builds." FORCE
)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING
    "Flags used by the shared libraries linker during coverage builds." FORCE
)
mark_as_advanced(
    CMAKE_CXX_FLAGS_COVERAGE CMAKE_C_FLAGS_COVERAGE CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE CMAKE_STATIC_LINKER_FLAGS_COVERAGE
)


find_program(LCOV_EXECUTABLE lcov)
find_program(GENINFO_EXECUTABLE geninfo)
find_program(GENHTML_EXECUTABLE genhtml)

set(LCOV_FIND_REQUIRED TRUE)
find_package_handle_standard_args(LCOV
    REQUIRED_VARS LCOV_EXECUTABLE GENINFO_EXECUTABLE GENHTML_EXECUTABLE
)
mark_as_advanced(LCOV_EXECUTABLE GENINFO_EXECUTABLE GENHTML_EXECUTABLE)


joynr_list_prepend_each(_coverage_excludes ' ${_coverage_excludes})
joynr_list_append_each(_coverage_excludes ' ${_coverage_excludes})

set(_coverage_extracts ${COVERAGE_EXTRACTS})

joynr_list_prepend_each(_coverage_extracts ' ${_coverage_extracts})
joynr_list_append_each(_coverage_extracts ' ${_coverage_extracts})


# Add coverage target.
add_custom_target(coverage
    COMMAND rm -f ${CMAKE_BINARY_DIR}/.tests-failed
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}

    # Compile sources first. (It is not possible to depend on a built-in target such as 'all':
    # https://cmake.org/Bug/view.php?id=8438)
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}

    # In order to get coverage for files that are not executed in any test, lcov has to be called
    # with --capture --initial before the tests are executed. This base needs to be combined with
    # the output of the --capture call after the tests.

    COMMAND ${LCOV_EXECUTABLE} --zerocounters --directory ${CMAKE_BINARY_DIR} --quiet
    COMMAND ${LCOV_EXECUTABLE} --capture --initial --no-external --quiet
                               --directory ${CMAKE_BINARY_DIR}
                               --base-directory ${COVERAGE_BASE_DIR}
                               --output-file ${COVERAGE_DIR}/${PROJECT_NAME}.coverage_base.info
                               ${_coverage_gcov_wrapper}

    # working escaping for make: \${ARGS} \$\${ARGS}
    # working escaping for ninja: \$\${ARGS}
    # No luck with VERBATIM option.
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure "\$\${ARGS}" || touch ${CMAKE_BINARY_DIR}/.tests-failed

    COMMAND ${LCOV_EXECUTABLE} --capture --no-external --quiet
                               --directory ${CMAKE_BINARY_DIR}
                               --output-file ${COVERAGE_DIR}/${PROJECT_NAME}.coverage_test.info
                               --base-directory ${COVERAGE_BASE_DIR}
                               --rc lcov_branch_coverage=${_coverage_enable_branch}
                               ${_coverage_gcov_wrapper}

    COMMAND ${LCOV_EXECUTABLE} --add-tracefile ${COVERAGE_DIR}/${PROJECT_NAME}.coverage_base.info
                               --add-tracefile ${COVERAGE_DIR}/${PROJECT_NAME}.coverage_test.info
                               --output-file ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                               --quiet
                               --rc lcov_branch_coverage=${_coverage_enable_branch}

    COMMAND ${LCOV_EXECUTABLE} --remove ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                               ${_coverage_excludes}
                               --output-file ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                               --quiet
                               --rc lcov_branch_coverage=${_coverage_enable_branch}

    COMMAND test -z "${COVERAGE_EXTRACTS}" ||
            ${LCOV_EXECUTABLE} --extract ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                               ${_coverage_extracts}
                               --output-file ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                               --quiet
                               --rc lcov_branch_coverage=${_coverage_enable_branch}

    COMMAND ${CMAKE_COMMAND} -P ${_coverage_script_folder}/PostProcessLcov.cmake
                             ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                             ${COVERAGE_BASE_DIR} ${CMAKE_CURRENT_BINARY_DIR}

    COMMAND ${GENHTML_EXECUTABLE} ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.info
                                  --output-directory ${COVERAGE_DIR}
                                  --show-details --legend --highlight --demangle-cpp
                                  --rc lcov_branch_coverage=${_coverage_enable_branch}
                                  | tee ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.log

    COMMAND ${CMAKE_COMMAND} -P ${_coverage_script_folder}/CheckThresholds.cmake
                                ${COVERAGE_DIR}/${PROJECT_NAME}.coverage.log
                                ${COVERAGE_THRESHOLD_LINE} ${COVERAGE_THRESHOLD_FUNCTION}
                                "" ${COVERAGE_THRESHOLD_BRANCH}

    COMMAND ${CMAKE_COMMAND} -E remove ${COVERAGE_DIR}/${PROJECT_NAME}.coverage_base.info
                                       ${COVERAGE_DIR}/${PROJECT_NAME}.coverage_test.info

    COMMAND ${CMAKE_COMMAND} -E echo "Coverage report: file://${COVERAGE_DIR}/index.html"

    # Fail the build if the tests failed.
    COMMAND test ! -f ${CMAKE_BINARY_DIR}/.tests-failed

    COMMENT "Generate code coverage"
    USES_TERMINAL # Ensure ninja outputs to stdout.
)

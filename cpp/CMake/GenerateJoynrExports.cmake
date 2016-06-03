include(CMakeParseArguments)

function(GenerateJoynrExports TARGET_NAME)
    set(options)
    set(oneValueArgs EXPORT_FILE_NAME)
    set(multiValueArgs)
    cmake_parse_arguments(GJE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT DEFINED GJE_EXPORT_FILE_NAME)
        message(FATAL_ERROR "Export file name location missing. Cannot generate header file.")
    endif()
    set(GENERATE_TEST_FILE ${GJE_EXPORT_FILE_NAME}_GJE_TEST_FILE)
    generate_export_header(
        ${TARGET_NAME}
        ${ARGN}
        EXPORT_FILE_NAME ${GENERATE_TEST_FILE}
    )
    exec_program("${CMAKE_COMMAND} -E copy_if_different ${GENERATE_TEST_FILE} ${GJE_EXPORT_FILE_NAME}")
    file(REMOVE ${GENERATE_TEST_FILE})
endfunction(GenerateJoynrExports)

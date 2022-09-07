# - Config file for the JoynrGenerator package
# It defines the following variables
#   JoynrGenerator_FOUND    - TRUE if the joynr generator was found successfully.
#   JoynrGenerator_JAR      - The executable jar file containing the joynr generator standalone.
#
# Function to call the joynr generator:
#   JoynrGenerator(OUTPUTPATH path
#                  MODELPATH path
#                  GENERATION_LANGUAGE <cpp|java|javascript>
#                  [GENERATION_ID id]
#                  [OUTPUT_HEADER_PATH dir]
#                  [ADD_VERSION_TO comment|package|none])

include(CMakeParseArguments)

if(IS_ABSOLUTE ${joynr.generator.standalone.jar})
    set(JoynrGenerator_JAR "${joynr.generator.standalone.jar}")
else()
    set(JoynrGenerator_JAR "${CMAKE_CURRENT_LIST_DIR}/${joynr.generator.standalone.jar}")
endif(IS_ABSOLUTE ${joynr.generator.standalone.jar})

if(EXISTS "${JoynrGenerator_JAR}")
    set(JoynrGenerator_FOUND TRUE)
else()
    set(JoynrGenerator_FOUND FALSE)
endif()

find_package(Java COMPONENTS Runtime REQUIRED)

function(JoynrGenerator)
    set(singleValueArgs
        OUTPUTPATH
        MODELPATH
        GENERATION_LANGUAGE
        GENERATION_ID
        OUTPUT_HEADER_PATH
        ADD_VERSION_TO
    )
    cmake_parse_arguments(joynrGenerator "" "${singleValueArgs}" "" ${ARGN})

    set(JAVA_OPTS
        -XX:InitialHeapSize=512m
        -XX:MaxHeapSize=1g
    )
    set(joynrGeneratorCmd "${JAVA_OPTS}" "-jar" "${JoynrGenerator_JAR}")

    if(joynrGenerator_MODELPATH)
        get_filename_component(joynrGenerator_ABSOLUTE_MODELPATH ${joynrGenerator_MODELPATH} ABSOLUTE)
        list(APPEND joynrGeneratorCmd "-modelPath" "${joynrGenerator_ABSOLUTE_MODELPATH}")
    endif()

    if(joynrGenerator_OUTPUTPATH)
        get_filename_component(joynrGenerator_ABSOLUTE_OUTPUTPATH ${joynrGenerator_OUTPUTPATH} ABSOLUTE)
        list(APPEND joynrGeneratorCmd "-outputPath" "${joynrGenerator_ABSOLUTE_OUTPUTPATH}")
    endif()

    if(joynrGenerator_GENERATION_LANGUAGE)
        list(APPEND joynrGeneratorCmd "-generationLanguage" "${joynrGenerator_GENERATION_LANGUAGE}")
    else()
        list(APPEND joynrGeneratorCmd "-generationLanguage" "cpp")
    endif()

    if(joynrGenerator_GENERATION_ID)
        list(APPEND joynrGeneratorCmd "-generationId" "${joynrGenerator_GENERATION_ID}")
    endif()

    if(joynrGenerator_OUTPUT_HEADER_PATH)
        get_filename_component(joynrGenerator_ABSOLUTE_OUTPUT_HEADER_PATH ${joynrGenerator_OUTPUT_HEADER_PATH} ABSOLUTE)
        list(APPEND joynrGeneratorCmd "-outputHeaderPath" "${joynrGenerator_ABSOLUTE_OUTPUT_HEADER_PATH}")
    endif()

    if(joynrGenerator_ADD_VERSION_TO)
        list(APPEND joynrGeneratorCmd "-addVersionTo" "${joynrGenerator_ADD_VERSION_TO}")
    endif()

    execute_process(
        COMMAND ${Java_JAVA_EXECUTABLE} ${joynrGeneratorCmd}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE joynrGenerator_EXIT_CODE
        OUTPUT_VARIABLE joynrGenerator_OUTPUT
        ERROR_VARIABLE joynrGenerator_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(joynrGenerator_EXIT_CODE)
        set(errorMsg "Failed to execute joynr generator.")
        set(errorMsg "${errorMsg}\njoynr generator command: ${joynrGeneratorCmd}")
        set(errorMsg "${errorMsg}\njoynr generator standard out and standard err:")
        set(errorMsg "${errorMsg}\n${joynrGenerator_OUTPUT}")
        message(FATAL_ERROR "${errorMsg}")
    endif()
endfunction(JoynrGenerator)

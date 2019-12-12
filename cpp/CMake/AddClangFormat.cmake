option(
    ENABLE_CLANG_FORMATTER
    "Use clang-formatter to format source code?"
    OFF
)
message(STATUS "option ENABLE_CLANG_FORMATTER=" ${ENABLE_CLANG_FORMATTER})

if(ENABLE_CLANG_FORMATTER)
    include(LocateProgram)
    LocateProgram(clang-format CLANG_FORMAT_PATH)
    if (NOT TARGET format)
        add_custom_target(format)
    endif (NOT TARGET format)
endif(ENABLE_CLANG_FORMATTER)

function(FilterSourcesBeforeFormatting INPUT OUTPUT EXCLUDE_PATTERN)
     set(FILTERED_LIST "")
     foreach(ENTRY ${${INPUT}})
       if(NOT ${ENTRY} MATCHES ${EXCLUDE_PATTERN})
           list(APPEND FILTERED_LIST ${ENTRY})
       endif()
     endforeach()
     set(${OUTPUT} ${FILTERED_LIST} PARENT_SCOPE)
endfunction(FilterSourcesBeforeFormatting)


function(AddClangFormat TARGET)
    if(${ENABLE_CLANG_FORMATTER})
        get_property(TARGET_SOURCES TARGET ${TARGET} PROPERTY SOURCES)
        set (ADDITIONAL_FUNCTION_ARGS ${ARGN})
        list(LENGTH ADDITIONAL_FUNCTION_ARGS NUM_ADDITIONAL_FUNCTION_ARGS)
        list(FILTER TARGET_SOURCES EXCLUDE REGEX "^\\\$<TARGET_OBJECTS:[^>]+>$")
        if (${NUM_ADDITIONAL_FUNCTION_ARGS} GREATER 0)
            list(GET ADDITIONAL_FUNCTION_ARGS 0 EXCLUDE_PATTERN)
            FilterSourcesBeforeFormatting(TARGET_SOURCES TARGET_SOURCES ${EXCLUDE_PATTERN})
        endif()
        set(COMMENT "running C++ code formatter: clang-format")
        set(COMMENT "${COMMENT}\nworking directory: ${CMAKE_CURRENT_SOURCE_DIR}")
        set(COMMENT "${COMMENT}\nprocessing files:\n${TARGET_SOURCES}")

        set(FORMAT_TARGET "format-${TARGET}")
        add_custom_target(
            ${FORMAT_TARGET}
            COMMAND "${CLANG_FORMAT_PATH}" -i ${TARGET_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
            COMMENT "${COMMENT}"
        )
        add_dependencies(${TARGET} ${FORMAT_TARGET})
        add_dependencies(format ${FORMAT_TARGET})
    endif(${ENABLE_CLANG_FORMATTER})
endfunction(AddClangFormat)

function(filter_out_headers INPUT OUTPUT)
     set(FILTERED_LIST "")
     foreach(ENTRY ${${INPUT}})
       if(NOT ${ENTRY} MATCHES ".+\\.h$")
           list(APPEND FILTERED_LIST ${ENTRY})
       endif()
     endforeach()
     set(${OUTPUT} ${FILTERED_LIST} PARENT_SCOPE)
endfunction(filter_out_headers)

function(AddClangTidy TARGET)
    if(${ENABLE_CLANG_TIDY})

        get_property(TARGET_SOURCES TARGET ${TARGET} PROPERTY SOURCES)
        filter_out_headers(TARGET_SOURCES TARGET_SOURCES_WO_HEADERS)

        set(TIDY_TARGET "tidy-${TARGET}")

        add_custom_target(
             ${TIDY_TARGET}
             COMMAND ${CLANG_TIDY_PATH} -p ${CMAKE_BINARY_DIR} ${CLANG_TIDY_OPTIONS} ${TARGET_SOURCES_WO_HEADERS}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             VERBATIM
        )
        add_dependencies(${TARGET} ${TIDY_TARGET})
    endif(${ENABLE_CLANG_TIDY})
endfunction(AddClangTidy)

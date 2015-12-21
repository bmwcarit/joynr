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
        list(APPEND ${CLANG_TIDY_TARGET_FILES} ${TARGET_SOURCES_WO_HEADERS})

        set(TIDY_TARGET "tidy-${TARGET}")
        add_custom_target(${TIDY_TARGET})
        add_dependencies(tidy ${TIDY_TARGET})

        foreach(TARGET_FILE ${TARGET_SOURCES_WO_HEADERS})
            set(TARGET_FILE_EXPORT_FILE "${CLANG_TIDY_FIXES_PATH}/${TARGET_FILE}.yaml")
            # make sure the folder exists
            get_filename_component(TARGET_FILE_EXPORT_FOLDER ${TARGET_FILE_EXPORT_FILE} DIRECTORY)
            file(MAKE_DIRECTORY ${TARGET_FILE_EXPORT_FOLDER})
            # call 'clang-tidy' for each source file of this target
            add_custom_command(
                 TARGET ${TIDY_TARGET}
                 COMMAND ${CLANG_TIDY_PATH} -p ${CMAKE_BINARY_DIR}
                                            -export-fixes ${TARGET_FILE_EXPORT_FILE}
                                            ${CLANG_TIDY_OPTIONS}
                                            ${TARGET_FILE}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                 VERBATIM
            )

        endforeach(TARGET_FILE ${TARGET_SOURCES_WO_HEADERS})
    endif(${ENABLE_CLANG_TIDY})

endfunction(AddClangTidy)

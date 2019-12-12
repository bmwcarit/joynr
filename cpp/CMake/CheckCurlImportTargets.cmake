if(NOT TARGET CURL::libcurl)
    message(STATUS "CURL::libcurl target not defined. Creating IMPORTED target.")
    message(STATUS "variable CURL_VERSION_STRING=${CURL_VERSION_STRING}")
    message(STATUS "variable CURL_INCLUDE_DIRS=${CURL_INCLUDE_DIRS}")
    message(STATUS "variable CURL_LIBRARIES=${CURL_LIBRARIES}")
    add_library(CURL::libcurl UNKNOWN IMPORTED)
    set_target_properties(CURL::libcurl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${CURL_LIBRARIES}"
    )
endif()

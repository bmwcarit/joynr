message(STATUS "variable CURL_FOUND=${CURL_FOUND}")
message(STATUS "variable CURL_VERSION_STRING=${CURL_VERSION_STRING}")
message(STATUS "variable CURL_INCLUDE_DIRS=${CURL_INCLUDE_DIRS}")
message(STATUS "variable CURL_LIBRARIES=${CURL_LIBRARIES}")

if(NOT TARGET cURL::curl)
    message(STATUS "cURL::curl target not defined. Creating IMPORTED target.")
    add_library(cURL::curl SHARED IMPORTED GLOBAL)
    set_target_properties(cURL::curl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${CURL_LIBRARIES}"
    )
endif(NOT TARGET cURL::curl)

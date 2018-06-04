message(STATUS "variable MOCOCRW_STRING=${MOCOCRW_VERSION}")
message(STATUS "variable MOCOCRW_INCLUDE_DIRS=${MOCOCRW_INCLUDE_DIRS}")
message(STATUS "variable MOCOCRW_LIBRARIES=${MOCOCRW_LIBRARIES}")

if(NOT TARGET MoCOCrW::mococrw)
    message(STATUS "MoCOCrW::mococrw target not defined. Creating IMPORTED target.")
    add_library(MoCOCrW::mococrw SHARED IMPORTED GLOBAL)
    set_property(TARGET MoCOCrW::mococrw PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${MOCOCRW_INCLUDE_DIRS}"
    )
    set_property(TARGET MoCOCrW::mococrw PROPERTY
        IMPORTED_LOCATION "${MOCOCRW_LIBRARIES}"
    )
add_dependencies(MoCOCrW::mococrw mococrw)
endif(NOT TARGET MoCOCrW::mococrw)

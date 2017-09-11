message(STATUS "variable MOCOCRW_STRING=${MOCOCRW_VERSION}")
message(STATUS "variable MOCOCRW_INCLUDE_DIRS=${MOCOCRW_INCLUDE_DIRS}")
message(STATUS "variable MOCOCRW_LIBRARIES=${MOCOCRW_LIBRARIES}")

if(NOT TARGET mococrw::mococrw)
    message(STATUS "mococrw::mococrw target not defined. Creating IMPORTED target.")
    add_library(mococrw::mococrw SHARED IMPORTED GLOBAL)
    set_property(TARGET mococrw::mococrw PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${MOCOCRW_INCLUDE_DIRS}"
    )
    set_property(TARGET mococrw::mococrw PROPERTY
        IMPORTED_LOCATION "${MOCOCRW_LIBRARIES}"
    )
add_dependencies(mococrw::mococrw mococrw)
endif(NOT TARGET mococrw::mococrw)

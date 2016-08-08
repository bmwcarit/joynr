message(STATUS "variable DLT_VERSION_STRING=${DLT_VERSION}")
message(STATUS "variable DLT_INCLUDE_DIRS=${DLT_INCLUDE_DIRS}")
message(STATUS "variable DLT_LIBRARIES=${DLT_LIBRARIES}")

if(NOT TARGET DLT::DLT)
    message(STATUS "DLT::DLT target not defined. Creating IMPORTED target.")
    add_library(DLT::DLT SHARED IMPORTED GLOBAL)
    set_property(TARGET DLT::DLT PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${DLT_INCLUDE_DIRS}"
    )
    set_property(TARGET DLT::DLT PROPERTY
        IMPORTED_LOCATION "${DLT_LIBRARIES}"
    )
add_dependencies(DLT::DLT DLT)
endif(NOT TARGET DLT::DLT)

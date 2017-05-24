message(STATUS "variable SMRF_STRING=${SMRF_VERSION}")
message(STATUS "variable SMRF_INCLUDE_DIRS=${SMRF_INCLUDE_DIRS}")
message(STATUS "variable SMRF_LIBRARIES=${SMRF_LIBRARIES}")

if(NOT TARGET smrf::smrf)
    message(STATUS "smrf::smrf target not defined. Creating IMPORTED target.")
    add_library(smrf::smrf SHARED IMPORTED GLOBAL)
    set_property(TARGET smrf::smrf PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${SMRF_INCLUDE_DIRS}"
    )
    set_property(TARGET smrf::smrf PROPERTY
        IMPORTED_LOCATION "${SMRF_LIBRARIES}"
    )
add_dependencies(smrf::smrf smrf)
endif(NOT TARGET smrf::smrf)

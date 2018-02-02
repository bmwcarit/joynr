message(STATUS "variable MOSQUITTO_VERSION_STRING=${MOSQUITTO_VERSION}")
message(STATUS "variable MOSQUITTO_INCLUDE_DIRS=${MOSQUITTO_INCLUDE_DIRS}")
message(STATUS "variable MOSQUITTO_LIBRARIES=${MOSQUITTO_LIBRARIES}")

if(NOT TARGET mosquitto::mosquitto)
    message(STATUS "mosquitto::mosquitto target not defined. Creating IMPORTED target.")
    add_library(mosquitto::mosquitto SHARED IMPORTED GLOBAL)
    set_property(TARGET mosquitto::mosquitto  PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${MOSQUITTO_INCLUDE_DIRS}
    )

    set_property(TARGET mosquitto::mosquitto  PROPERTY
        IMPORTED_LOCATION ${MOSQUITTO_LIBRARIES}
    )
    add_dependencies(mosquitto::mosquitto mosquitto)
endif(NOT TARGET mosquitto::mosquitto)


find_path(MOSQUITTO_INCLUDE_DIR mosquittopp.h)
find_library(MOSQUITTO_LIBRARY libmosquittopp.so)

set(MOSQUITTO_INCLUDE_DIRS ${MOSQUITTO_INCLUDE_DIR} )
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mosquitto DEFAULT_MSG
    MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)

mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARY)


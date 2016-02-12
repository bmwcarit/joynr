find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)
mark_as_advanced(GMOCK_INCLUDE_DIR)

find_library(
    GMOCK_LIBRARY
    NAMES gmock
)

mark_as_advanced(GMOCK_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMock DEFAULT_MSG GMOCK_LIBRARY GMOCK_INCLUDE_DIR)

if(GMOCK_FOUND)
  set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR})
  set(GMOCK_LIBRARIES ${GMOCK_LIBRARY})
endif(GMOCK_FOUND)

find_path(jsmn_INCLUDE_DIR jsmn.h)
find_library(jsmn_LIBRARY libjsmn.a)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jsmn DEFAULT_MSG
    jsmn_LIBRARY jsmn_INCLUDE_DIR)

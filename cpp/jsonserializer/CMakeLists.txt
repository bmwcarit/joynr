message(STATUS "#############################################")
message(STATUS "Configuring jsonserializer...")
message(STATUS "#############################################\n")

### Add jsmn ###########################################################
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

ExternalProject_Add(
    jsmn
    GIT_REPOSITORY https://github.com/zserge/jsmn
    TIMEOUT 10
    # Disable svn update
    UPDATE_COMMAND ""
    PATCH_COMMAND rm -f config.mk COMMAND git apply ${CMAKE_SOURCE_DIR}/CMake/jsmn-fpic.patch
    # Disable configure step
    CONFIGURE_COMMAND ""
    # Disable build step
    BUILD_COMMAND "make"
    BUILD_IN_SOURCE 1
    # Disable install step
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    CMAKE_CACHE_ARGS "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
                     "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
)

ExternalProject_Get_Property(jsmn SOURCE_DIR)
set(jsmn_SOURCE_DIR ${SOURCE_DIR})

set_target_properties(
    jsmn
    PROPERTIES
    PUBLIC_INCLUDE_DIRECTORY "${jsmn_SOURCE_DIR}"
)

# Require C++11 when C++ compiling
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Weffc++ -fno-rtti")

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)

set(
    jsonserializer_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${jsmn_SOURCE_DIR}
)

set(
    jsonserializer_INCLUDE_DIRECTORIES
    ${jsonserializer_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${jsmn_SOURCE_DIR}
)

file(
    GLOB_RECURSE jsonserializer_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

file(
    GLOB jsonserializer_PRIVATE_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h"
    "exceptions/*.h"
)

set(
    jsonserializer_HEADERS
    ${jsonserializer_PUBLIC_HEADERS}
    ${jsonserializer_PRIVATE_HEADERS}
)

file(
    GLOB_RECURSE jsonserializer_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
)

add_library(
    jsonserializer SHARED
    ${jsonserializer_HEADERS}
    ${jsonserializer_SOURCES}
)

include_directories(
    ${jsonserializer_INCLUDE_DIRECTORIES}
)

SET(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where lib will install")
SET(CMAKE_MODULES_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/jsonserializer" CACHE PATH "The directory to install find modules to")

message(STATUS "jsonserializer LIB_INSTALL_DIR=${LIB_INSTALL_DIR}")
message(STATUS "jsonserializer CMAKE_MODULES_INSTALL_DIR=${CMAKE_MODULES_INSTALL_DIR}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib${LIB_SUFFIX}" )
message(STATUS "jsonserializer LIBRARY_OUTPUT_PATH=${LIBRARY_OUTPUT_PATH}")

target_link_libraries(jsonserializer JoynrCommon ${jsmn_SOURCE_DIR}/libjsmn.a)

set_target_properties(
    jsonserializer
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${jsonserializer_PUBLIC_INCLUDE_DIRECTORY}"
)

add_dependencies(jsonserializer common jsmn)

install(
    TARGETS jsonserializer EXPORT JoynrTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT library
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT development
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY .
    DESTINATION "${INSTALL_INCLUDE_DIR}/jsonserializer"
    FILES_MATCHING PATTERN "*.h"
)

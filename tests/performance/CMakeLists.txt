cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(performance-test-0.16.0-SNAPSHOT)

# Compile to a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

# This flag will be set automatically by CMake based on "CMAKE_CXX_STANDARD" starting with version 3.1.0.
# For older versions, this flag is set manually for gcc and clang:
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_VERSION}" VERSION_LESS 3.1.0)
    if( (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    endif( (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
endif("${CMAKE_VERSION}" VERSION_LESS 3.1.0)

option(
    ENABLE_CLANG_FORMATTER
    "Use clang-formatter to format source code?"
    ON
)
message(STATUS "option ENABLE_CLANG_FORMATTER=" ${ENABLE_CLANG_FORMATTER})

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 0.16.0 REQUIRED)
message(STATUS "joynr variable Joynr_LIB_COMMON_INCLUDE_DIRS=${Joynr_LIB_COMMON_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_INPROCESS_INCLUDE_DIRS=${Joynr_LIB_INPROCESS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_DBUS_INCLUDE_DIRS=${Joynr_LIB_DBUS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_WS_INCLUDE_DIRS=${Joynr_LIB_WS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_COMMON_LIBRARIES=${Joynr_LIB_COMMON_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_INPROCESS_LIBRARIES=${Joynr_LIB_INPROCESS_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_DBUS_LIBRARIES=${Joynr_LIB_DBUS_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_WS_LIBRARIES=${Joynr_LIB_WS_LIBRARIES}")
message(STATUS "joynr variable Joynr_EXECUTABLES=${Joynr_EXECUTABLES}")
message(STATUS "joynr variable Joynr_RESOURCES_DIR=${Joynr_RESOURCES_DIR}")
message(STATUS "joynr variable USE_DBUS_COMMONAPI_COMMUNICATION=${USE_DBUS_COMMONAPI_COMMUNICATION}")

######## Add Boost ########

find_package(Boost 1.56.0 REQUIRED
    COMPONENTS
    filesystem
)
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})

##### Helper variables for using generated code ###########

file(GLOB_RECURSE GENERATED_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-cpp/include/*.h"
)

file(GLOB_RECURSE GENERATED_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-cpp/*.cpp"
)

set(GENERATED_INCLUDE_DIRS
    "src/main/generated-cpp/include"
)

# Put the Joynr_LIB_COMMON_INCLUDE_DIRS last so that it is possible to override types
# defined in libjoynr
include_directories(
    ${GENERATED_INCLUDE_DIRS}
    ${Joynr_LIB_COMMON_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

set(PROVIDER_HEADERS
    "src/main/cpp/PerformanceTestEchoProvider.h"
)

set(PROVIDER_SOURCES
    "src/main/cpp/PerformanceTestEchoProvider.cpp"
    "src/main/cpp/PerformanceTestApplication.cpp"
)

set(PROVIDER_LIBRARIES
    ${Joynr_LIB_COMMON_LIBRARIES}
    ${Joynr_LIB_WS_LIBRARIES}
)

add_executable(performancetest-provider-ws
    ${GENERATED_SOURCES}
    ${GENERATED_HEADERS}
    ${PROVIDER_SOURCES}
    ${PROVIDER_HEADERS}
)

target_link_libraries(performancetest-provider-ws
    ${PROVIDER_LIBRARIES}
    ${Joynr_LIB_WS_LIBRARIES}
    ${Boost_LIBRARIES}
)

install(TARGETS
    performancetest-provider-ws
    RUNTIME DESTINATION bin COMPONENT bin
)

if(ENABLE_CLANG_FORMATTER)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cpp/CMake")
    include(LocateProgram)
    LocateProgram(clang-format CLANG_FORMAT_PATH)
    include(AddClangFormat)
    add_custom_target(format)

    AddClangFormat(performancetest-provider-ws)
endif(ENABLE_CLANG_FORMATTER)

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/performancetest-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/performancetest-provider.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc-default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc-default-messaging.settings)

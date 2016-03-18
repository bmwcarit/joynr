cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(performance-test-0.16.0)

# Compile to a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cpp/CMake")
include(AddClangFormat)

if(ENABLE_CLANG_FORMATTER)
    include(LocateProgram)
    LocateProgram(clang-format CLANG_FORMAT_PATH)
    add_custom_target(format)
endif(ENABLE_CLANG_FORMATTER)

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 0.16.0 REQUIRED)

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)

# set log level to FATAL to disable most log messages
add_definitions(-DJOYNR_MAX_LOG_LEVEL_FATAL)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/performancetest-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/performancetest-provider.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc-default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc-default-messaging.settings)

######## Add Boost ########

find_package(Boost 1.56.0 REQUIRED
    COMPONENTS
    filesystem
)
include_directories(${Boost_INCLUDE_DIRS})

##### Library consisting of the generated code ###########

file(GLOB_RECURSE PERFORMANCE_GENERATED_HEADERS
    "src/main/generated-cpp/include/*.h"
)

file(GLOB_RECURSE PERFORMANCE_GENERATED_SOURCES
    "src/main/generated-cpp/*.cpp"
)

add_library(performance-generated SHARED
    ${PERFORMANCE_GENERATED_HEADERS}
    ${PERFORMANCE_GENERATED_SOURCES}
)

target_link_libraries(performance-generated
    ${Joynr_LIB_COMMON_LIBRARIES}
)

target_include_directories(performance-generated
    SYSTEM PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/src/main/generated-cpp/include"
    ${Joynr_LIB_COMMON_INCLUDE_DIRS}
)

### provider implementation ###
add_subdirectory(src/main/cpp/provider)

### provider application ###
add_subdirectory(src/main/cpp/provider-app)

add_subdirectory(src/main/cpp/short-circuit)

add_subdirectory(src/main/cpp/serializer)

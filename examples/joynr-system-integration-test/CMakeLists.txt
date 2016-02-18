cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(jsit-0.16.0-SNAPSHOT)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

# set required C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 0.16.0 REQUIRED)
message(STATUS "joynr variable Joynr_LIB_COMMON_INCLUDE_DIRS=${Joynr_LIB_COMMON_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_WS_INCLUDE_DIRS=${Joynr_LIB_WS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_COMMON_LIBRARIES=${Joynr_LIB_COMMON_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_WS_LIBRARIES=${Joynr_LIB_WS_LIBRARIES}")
message(STATUS "joynr variable Joynr_EXECUTABLES=${Joynr_EXECUTABLES}")
message(STATUS "joynr variable Joynr_RESOURCES_DIR=${Joynr_RESOURCES_DIR}")

option(
    ENABLE_CLANG_FORMATTER
    "Use clang-formatter to format source code?"
    ON
)
message(STATUS "option ENABLE_CLANG_FORMATTER=" ${ENABLE_CLANG_FORMATTER})

if(ENABLE_CLANG_FORMATTER)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cpp/CMake")
    include(LocateProgram)
    LocateProgram(clang-format CLANG_FORMAT_PATH)
    include(AddClangFormat)
    add_custom_target(format)
endif(ENABLE_CLANG_FORMATTER)

##### Helper variables for using generated code ###########
file(
    GLOB_RECURSE
    GENERATED_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-cpp/include/*.h"
)

file(
    GLOB_RECURSE
    GENERATED_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-cpp/*.cpp"
)

set(
    GENERATED_INCLUDE_DIRS
    "src/main/generated-cpp/include"
)

# Put the Joynr_LIB_COMMON_INCLUDE_DIRS last so that it is possible to override types
# defined in libjoynr
include_directories(
    ${GENERATED_INCLUDE_DIRS}
    ${Joynr_LIB_COMMON_INCLUDE_DIRS}
)

# Consumer headers
set(
    jsit-consumer_HEADERS
)

# Consumer sources
set(
    jsit-consumer_SOURCES
    "src/main/cpp/ConsumerApplication.cpp"
)

# Provider headers
set(
    jsit-provider_HEADERS
    ${GENERATED_HEADERS}
    "src/main/cpp/SystemIntegrationTestProvider.h"
)

# Provider sources
set(
    jsit-provider_SOURCES
    "src/main/cpp/SystemIntegrationTestProvider.cpp"
    "src/main/cpp/ProviderApplication.cpp"
)

# Link with the joynr libraries
set(
    jsit_LIBRARIES
    ${Joynr_LIB_COMMON_LIBRARIES}
)

# Put the generated code into a library and put the library in the bin directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_library(
    jsit SHARED
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

target_link_libraries(
    jsit
    ${jsit_LIBRARIES}
)

# copy joynr resources and settings
file(
    COPY ${Joynr_RESOURCES_DIR}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# copy joynr executables
file(
    COPY ${Joynr_EXECUTABLES}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Compile the provider application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(
    jsit-provider-ws
    ${jsit-provider_HEADERS}
    ${jsit-provider_SOURCES}
)

# Link the provider application
target_link_libraries(
    jsit-provider-ws
    jsit
    ${jsit_LIBRARIES}
    ${Joynr_LIB_WS_LIBRARIES}
)

# Compile the consumer application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(
    jsit-consumer-ws
    ${jsit-consumer_HEADERS}
    ${jsit-consumer_SOURCES}
)

# Link the consumer application
target_link_libraries(
    jsit-consumer-ws
    jsit
    ${jsit_LIBRARIES}
    ${Joynr_LIB_WS_LIBRARIES}
)

install(
    TARGETS
    jsit-provider-ws
    jsit-consumer-ws
    RUNTIME DESTINATION bin
)

if(ENABLE_CLANG_FORMATTER)
    AddClangFormat(jsit)
    AddClangFormat(jsit-consumer-ws)
    AddClangFormat(jsit-provider-ws)
endif(ENABLE_CLANG_FORMATTER)

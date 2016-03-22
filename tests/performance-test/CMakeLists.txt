cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(performance-test-0.17.0-SNAPSHOT)

# Compile to a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")
include(SetCppStandard)
include(Sanitizers)
include(AddClangFormat)

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 0.17.0 REQUIRED)

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)

if(NOT DEFINED JOYNR_SERVER_HTTP_PORT)
    set(JOYNR_SERVER_HTTP_PORT "8080")
endif(NOT DEFINED JOYNR_SERVER_HTTP_PORT)
message(STATUS "variable JOYNR_SERVER_HTTP_PORT=" ${JOYNR_SERVER_HTTP_PORT})

if(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
    set(JOYNR_SERVER_MQTT_PORT "1883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
message(STATUS "variable JOYNR_SERVER_MQTT_PORT=" ${JOYNR_SERVER_MQTT_PORT})

# set log level to FATAL to disable most log messages
add_definitions(-DJOYNR_MAX_LOG_LEVEL_FATAL)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/performancetest-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/performancetest-provider.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc-default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc-default-messaging.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/cluster-controller/resources/default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/common/resources/default-system-services.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-system-services.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/cluster-controller/resources/default-websocket.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings)

######## Add Boost ########

find_package(Boost 1.56.0 REQUIRED
    COMPONENTS
    filesystem
    program_options
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

add_subdirectory(src/main/cpp/consumer-app)

add_subdirectory(src/main/cpp/short-circuit)

add_subdirectory(src/main/cpp/serializer)

add_subdirectory(src/main/cpp/memory-usage)

# copy joynr resources and settings
file(
    COPY ${Joynr_RESOURCES_DIR}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

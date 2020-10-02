cmake_minimum_required(VERSION 3.10)

######## Project settings ########
project(inter-language-test-1.15.2)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")

include(AddClangFormat)
include(Sanitizers)

set(
    JOYNR_SERVER
    "localhost:8080"
    CACHE STRING "joynr infrastructure server (BounceProxy, CapDir, ChannelUrlDir) to use in settings files."
)

message(STATUS "option JOYNR_SERVER=${JOYNR_SERVER}")

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 1.15.2 REQUIRED HINTS "${CMAKE_BINARY_DIR}/../joynr/bin")
find_package(CURL REQUIRED)
find_package(mosquitto 1.6.8 REQUIRED)
include(CheckMosquittoImportTargets)

message(STATUS "joynr variable Joynr_EXECUTABLES=${Joynr_EXECUTABLES}")
message(STATUS "joynr variable Joynr_RESOURCES_DIR=${Joynr_RESOURCES_DIR}")

### BEGIN Boost ###

find_package(Boost 1.58.0 REQUIRED
    COMPONENTS
    system
    filesystem
)
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})

### END Boost ###

include(AddGtestGmock)

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

file(GLOB_RECURSE GENERATED_HEADERS_PROXY_PROVIDER_INTERFACE_MISMATCH RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-cpp-proxy-provider-interface-mismatch/include/*.h"
)

file(GLOB_RECURSE GENERATED_SOURCES_PROXY_PROVIDER_INTERFACE_MISMATCH RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-cpp-proxy-provider-interface-mismatch/*.cpp"
)

set(GENERATED_INCLUDE_DIRS_PROXY_PROVIDER_INTERFACE_MISMATCH
    "src/main/generated-cpp-proxy-provider-interface-mismatch/include"
)

# Use the generated header files for the consumer application
set(ilt-consumer_HEADERS
    "src/main/cpp/IltHelper.h"
    "src/main/cpp/IltUtil.h"
    "src/main/cpp/IltAbstractConsumerTest.h"
)

# Use the generated source files for the consumer application
set(ilt-consumer_SOURCES
    "src/main/cpp/IltHelper.cpp"
    "src/main/cpp/IltConsumerAsyncMethodTest.cpp"
    "src/main/cpp/IltConsumerAttributeSubscriptionTest.cpp"
    "src/main/cpp/IltConsumerBroadcastSubscriptionTest.cpp"
    "src/main/cpp/IltConsumerCompressionTest.cpp"
    "src/main/cpp/IltConsumerFilteredBroadcastSubscriptionTest.cpp"
    "src/main/cpp/IltConsumerFireAndForgetMethodTest.cpp"
    "src/main/cpp/IltConsumerGetterSetterTest.cpp"
    "src/main/cpp/IltConsumerSyncMethodTest.cpp"
    "src/main/cpp/IltAbstractConsumerTest.cpp"
)

# Use the generated header files for the consumer application
set(ilt-consumer-proxy-provider-interface-mismatch_HEADERS
    ${GENERATED_HEADERS_PROXY_PROVIDER_INTERFACE_MISMATCH}
    "src/main/cpp/IltHelper.h"
    "src/main/cpp/IltUtil.h"
    "src/main/cpp/IltAbstractConsumerTest.h"
)

# Use the generated source files for the consumer application
set(ilt-consumer-proxy-provider-interface-mismatch_SOURCES
    ${GENERATED_SOURCES_PROXY_PROVIDER_INTERFACE_MISMATCH}
    "src/main/cpp/IltHelper.cpp"
    "src/main/cpp/IltConsumerMethodInvocationExcpetionTest.cpp"
    "src/main/cpp/IltAbstractConsumerTest.cpp"
)

# Use the generated header files for the provider application
set(ilt-provider_HEADERS
    ${GENERATED_HEADERS}
    "src/main/cpp/IltProvider.h"
    "src/main/cpp/IltHelper.h"
    "src/main/cpp/IltUtil.h"
    "src/main/cpp/IltStringBroadcastFilter.h"
)

# Use the generated source files for the provider application
set(ilt-provider_SOURCES
    "src/main/cpp/IltProvider.cpp"
    "src/main/cpp/IltHelper.cpp"
    "src/main/cpp/IltProviderApplication.cpp"
    "src/main/cpp/IltStringBroadcastFilter.cpp"
)

# Link with the joynr libraries
set(ilt_LIBRARIES
    Joynr::JoynrLib
    ${Boost_LIBRARIES}
)

# Put the generated code into a library and put the library in the bin directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_library(ilt SHARED
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

target_include_directories(
    ilt
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

target_link_libraries(ilt
    ${ilt_LIBRARIES}
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

# Compile the provider application (running in-process with cluster controller)
add_executable(ilt-provider-cc
    ${ilt-provider_HEADERS}
    ${ilt-provider_SOURCES}
)

# Link the provider application
target_link_libraries(ilt-provider-cc
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrClusterControllerRuntime
)

target_include_directories(
    ilt-provider-cc
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Compile the provider application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(ilt-provider-ws
    ${ilt-provider_HEADERS}
    ${ilt-provider_SOURCES}
)

target_include_directories(
    ilt-provider-ws
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Link the provider application
target_link_libraries(ilt-provider-ws
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrWsRuntime
)

# Compile the provider application (requires: running cluster controller)
# Communicates via Unix Domain Socket with cluster controller
add_executable(ilt-provider-uds
    ${ilt-provider_HEADERS}
    ${ilt-provider_SOURCES}
)

target_include_directories(
    ilt-provider-uds
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Link the provider application
target_link_libraries(ilt-provider-uds
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrUdsRuntime
)

# Compile the test application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
AddTest(
    ilt-consumer-ws
    ${ilt-consumer_HEADERS}
    ${ilt-consumer_SOURCES}
)

target_include_directories(
    ilt-consumer-ws
    PRIVATE ${GENERATED_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests
)

# Link the consumer application
target_link_libraries(ilt-consumer-ws
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrWsRuntime
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Compile the extended test application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
AddTest(
    ilt-consumer-proxy-provider-interface-mismatch-ws
    ${ilt-consumer-proxy-provider-interface-mismatch_HEADERS}
    ${ilt-consumer-proxy-provider-interface-mismatch_SOURCES}
)

target_include_directories(
    ilt-consumer-proxy-provider-interface-mismatch-ws
    PRIVATE ${GENERATED_INCLUDE_DIRS_PROXY_PROVIDER_INTERFACE_MISMATCH}
    PRIVATE ${GENERATED_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests
)

# Link the consumer application
target_link_libraries(ilt-consumer-proxy-provider-interface-mismatch-ws
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrWsRuntime
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Compile the test application (requires: running cluster controller)
# Communicates via Unix Domain Sockets with cluster controller
AddTest(
    ilt-consumer-uds
    ${ilt-consumer_HEADERS}
    ${ilt-consumer_SOURCES}
)

target_include_directories(
    ilt-consumer-uds
    PRIVATE ${GENERATED_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests
)

# Link the consumer application
target_link_libraries(ilt-consumer-uds
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrUdsRuntime
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Compile the extended test application (requires: running cluster controller)
# Communicates via Unix Domain Socket with cluster controller
AddTest(
    ilt-consumer-proxy-provider-interface-mismatch-uds
    ${ilt-consumer-proxy-provider-interface-mismatch_HEADERS}
    ${ilt-consumer-proxy-provider-interface-mismatch_SOURCES}
)

target_include_directories(
    ilt-consumer-proxy-provider-interface-mismatch-uds
    PRIVATE ${GENERATED_INCLUDE_DIRS_PROXY_PROVIDER_INTERFACE_MISMATCH}
    PRIVATE ${GENERATED_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests
)

# Link the consumer application
target_link_libraries(ilt-consumer-proxy-provider-interface-mismatch-uds
    ilt
    ${ilt_LIBRARIES}
    Joynr::JoynrUdsRuntime
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

install(TARGETS
            ilt-provider-ws
            ilt-consumer-ws
            ilt-provider-cc
            ilt-provider-uds
            ilt-consumer-uds
        RUNTIME DESTINATION bin COMPONENT bin
)

AddClangFormat(ilt-provider-cc)
AddClangFormat(ilt-provider-ws)
AddClangFormat(ilt-consumer-ws)
AddClangFormat(ilt-provider-uds)
AddClangFormat(ilt-consumer-uds)

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)

if(NOT DEFINED JOYNR_SERVER_HTTP_PORT)
    set(JOYNR_SERVER_HTTP_PORT "8080")
endif(NOT DEFINED JOYNR_SERVER_HTTP_PORT)

if(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
    set(JOYNR_SERVER_MQTT_PORT "1883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_PORT)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/ilt-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/ilt-provider.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/ilt-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/ilt-consumer.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.mqtt.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.mqtt.messaging.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.http.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.http.messaging.settings)

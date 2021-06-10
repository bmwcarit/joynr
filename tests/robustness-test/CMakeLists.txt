cmake_minimum_required(VERSION 3.10)

######## Project settings ########
project(robustness-tests VERSION 1.17.2 LANGUAGES CXX)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")

##### cmake options #####################

option(
    GENERATE_SOURCES
    "Generate source files from Franca model?"
    OFF
)
message(STATUS "option GENERATE_SOURCES=" ${GENERATE_SOURCES})

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 1.17.2 REQUIRED HINTS "${CMAKE_BINARY_DIR}/../joynr/bin")
find_package(CURL REQUIRED)
find_package(mosquitto 1.6.8 REQUIRED)
include(CheckMosquittoImportTargets)

if(${GENERATE_SOURCES})
    find_package(JoynrGenerator ${Joynr_VERSION} EXACT REQUIRED)
    message(STATUS "variable JoynrGenerator_FOUND=${JoynrGenerator_FOUND}")
    message(STATUS "variable JoynrGenerator_JAR=${JoynrGenerator_JAR}")
    JoynrGenerator(
        OUTPUTPATH ${CMAKE_SOURCE_DIR}/src/main/generated-cpp
        MODELPATH ${CMAKE_SOURCE_DIR}/src/main/model/radio.fidl
        GENERATION_LANGUAGE "cpp"
    )
endif(${GENERATE_SOURCES})

include(AddClangFormat)

######## Add Boost ########

find_package(Boost 1.58.0 REQUIRED
    COMPONENTS
    system
    filesystem
)
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})
include(AddGtestGmock)

##### Helper variables for using generated code ###########

file(GLOB_RECURSE GENERATED_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "../robustness-test-base/src/main/generated-cpp/include/*.h"
)

file(GLOB_RECURSE GENERATED_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "../robustness-test-base/src/main/generated-cpp/*.cpp"
)

file(GLOB_RECURSE TEST_SCRIPTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.sh"
)

set(GENERATED_INCLUDE_DIRS
    "../robustness-test-base/src/main/generated-cpp/include"
)

# Put the Joynr_LIB_COMMON_INCLUDE_DIRS last so that it is possible to override types
# defined in libjoynr


# Use the generated header files for the consumer application
set(robustness-tests-consumer_HEADERS
    "src/main/cpp/AbstractRobustnessTest.h"
)

# Use the generated source files for the consumer application
set(robustness-tests-consumer_SOURCES
    "src/main/cpp/AbstractRobustnessTest.cpp"
    "src/main/cpp/RobustnessTestCcCrash.cpp"
    "src/main/cpp/RobustnessTestProviderCrash.cpp"
)

# Use the generated header files for the consumer application
set(robustness-tests-consumer-mqtt_HEADERS
    "src/main/cpp/RobustnessTestMqttConnectionReset.h"
)

# Use the generated source files for the consumer application
set(robustness-tests-consumer-mqtt_SOURCES
    "src/main/cpp/RobustnessTestMqttConnectionReset.cpp"
)

# Use the generated header files for the provider application
set(robustness-tests-provider_HEADERS
    ${GENERATED_HEADERS}
    "src/main/cpp/RobustnessTestProvider.h"
)

# Use the generated source files for the provider application
set(robustness-tests-provider_SOURCES
    "src/main/cpp/RobustnessTestProvider.cpp"
    "src/main/cpp/RobustnessTestProviderApplication.cpp"
)

# Link with the joynr libraries
set(robustness-tests_LIBRARIES
    Joynr::JoynrLib
    ${Boost_LIBRARIES}
)

list(
    APPEND robustness-tests_LIBRARIES
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Put the generated code into a library
add_library(robustness-tests-generated SHARED
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

target_include_directories(
    robustness-tests-generated
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

if(NOT USE_PLATFORM_GTEST_GMOCK)
    # make sure the external projects googletest and googlemock is built before this
    add_dependencies(robustness-tests-generated googletest googlemock)
endif(NOT USE_PLATFORM_GTEST_GMOCK)

target_link_libraries(robustness-tests-generated
    ${robustness-tests_LIBRARIES}
)

# Compile the provider application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(robustness-tests-provider-ws
    ${robustness-tests-provider_HEADERS}
    ${robustness-tests-provider_SOURCES}
)

target_include_directories(
    robustness-tests-provider-ws
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Link the provider application
target_link_libraries(robustness-tests-provider-ws
    robustness-tests-generated
    ${robustness-tests_LIBRARIES}
    Joynr::JoynrWsRuntime
)

#Compile Provider-cc
add_executable(robustness-tests-provider-cc
    ${robustness-tests-provider_HEADERS}
    ${robustness-tests-provider_SOURCES}
)

target_include_directories(
    robustness-tests-provider-cc
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Link the provider-cc
target_link_libraries(robustness-tests-provider-cc
    robustness-tests-generated
    ${robustness-tests_LIBRARIES}
    Joynr::JoynrClusterControllerRuntime
)

# Compile the test application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
AddTest(
    robustness-tests-ws
    ${robustness-tests-consumer_HEADERS}
    ${robustness-tests-consumer_SOURCES}
)

target_include_directories(
    robustness-tests-ws
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Link the test application
target_link_libraries(robustness-tests-ws
    robustness-tests-generated
    ${robustness-tests_LIBRARIES}
    Joynr::JoynrWsRuntime
)

# Compile the test application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
AddTest(
    robustness-tests-consumer-mqtt-cc
    ${robustness-tests-consumer-mqtt_HEADERS}
    ${robustness-tests-consumer-mqtt_SOURCES}
)

target_include_directories(
    robustness-tests-consumer-mqtt-cc
    PRIVATE ${GENERATED_INCLUDE_DIRS}
)

# Link the test application
target_link_libraries(robustness-tests-consumer-mqtt-cc
    robustness-tests-generated
    ${robustness-tests_LIBRARIES}
    Joynr::JoynrClusterControllerRuntime
)

# copy runtime dependencies to output directory
file(
    COPY
    ${Joynr_RESOURCES_DIR}
    ${Joynr_EXECUTABLES}
    ${TEST_SCRIPTS}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

install(TARGETS
            robustness-tests-provider-ws
            robustness-tests-provider-cc
            robustness-tests-ws
            robustness-tests-consumer-mqtt-cc
        RUNTIME DESTINATION bin COMPONENT bin
)

AddClangFormat(robustness-tests-provider-ws)
AddClangFormat(robustness-tests-ws)
AddClangFormat(robustness-tests-provider-cc)
AddClangFormat(robustness-tests-consumer-mqtt-cc)

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)

if(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
    set(JOYNR_SERVER_MQTT_PORT "1883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_PORT)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-resources/robustness-tests-provider.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/robustness-tests-provider.settings
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-resources/robustness-tests-consumer.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/robustness-tests-consumer.settings
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-resources/cc.messaging.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.messaging.settings
    @ONLY
)

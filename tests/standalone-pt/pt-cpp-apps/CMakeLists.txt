cmake_minimum_required(VERSION 3.10)

######## Project settings ########
project(pt-cpp-apps VERSION 1.21.0 LANGUAGES CXX)

# Compile to a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/CMake")
include(Sanitizers)
include(AddClangFormat)

include(GNUInstallDirs)

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 1.21.0 REQUIRED HINTS "${CMAKE_BINARY_DIR}/../joynr/bin")

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)

if(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
    set(JOYNR_SERVER_MQTT_PORT "1883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
message(STATUS "variable JOYNR_SERVER_MQTT_PORT=" ${JOYNR_SERVER_MQTT_PORT})

# set log level to FATAL to disable most log messages
add_definitions(-DJOYNR_MAX_LOG_LEVEL_FATAL)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/performancetest-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/performancetest-consumer.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/libjoynrclustercontroller/resources/default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/libjoynr/resources/default-system-services.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-system-services.settings)

######## Add Boost ########
find_package(Boost 1.65.0 REQUIRED
    COMPONENTS
    filesystem
    program_options
)
include_directories(${Boost_INCLUDE_DIRS})

######## Add dependency to dummyKeychain ########
find_package(dummyKeychain 0.0.1 REQUIRED)
message(STATUS "Found dummyKeychain version ${dummyKeychain_VERSION}")

######## Add Mosquitto ########
find_package(mosquitto 1.6.8 REQUIRED)
include(CheckMosquittoImportTargets)

add_subdirectory(src/main/cpp/consumer-app)

# copy joynr resources and settings
file(
    COPY ${Joynr_RESOURCES_DIR}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

if(NOT INSTALL_TEST_DIR)
    set(INSTALL_TEST_DIR "${CMAKE_INSTALL_BINDIR}" CACHE PATH "Installation sub directory for tests")
endif(NOT INSTALL_TEST_DIR)

install(
    FILES
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/performancetest-consumer.settings
        DESTINATION ${INSTALL_TEST_DIR}/resources
)

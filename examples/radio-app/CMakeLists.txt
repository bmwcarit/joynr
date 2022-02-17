cmake_minimum_required(VERSION 3.10)

######## Project settings ########
project(radio-app VERSION 1.19.8 LANGUAGES CXX)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")
include(AddClangFormat)

set(
    JOYNR_SERVER
    "localhost:8080"
    CACHE STRING "joynr infrastructure server (BounceProxy, CapDir) to use in settings files."
)

message(STATUS "option JOYNR_SERVER=${JOYNR_SERVER}")

option(
    GENERATE_SOURCES
    "Generate source files from Franca model?"
    OFF
)
message(STATUS "option GENERATE_SOURCES=" ${GENERATE_SOURCES})

option(
    ENABLE_CCACHE
    "Should ccache be used?"
    OFF
)
message(STATUS "option ENABLE_CCACHE=" ${ENABLE_CCACHE})

if(ENABLE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_FOUND})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_FOUND})
    else(CCACHE_FOUND)
        message(WARNING "ccache was not found. Fall-back to default compiler.")
    endif(CCACHE_FOUND)
endif(ENABLE_CCACHE)

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 1.19.8 REQUIRED HINTS "${CMAKE_BINARY_DIR}/../joynr/bin")
message(STATUS "joynr variable Joynr_VERSION=${Joynr_VERSION}")
message(STATUS "joynr variable Joynr_EXECUTABLES=${Joynr_EXECUTABLES}")
message(STATUS "joynr variable Joynr_RESOURCES_DIR=${Joynr_RESOURCES_DIR}")

find_package(mosquitto 1.6.8 REQUIRED)
include(CheckMosquittoImportTargets)
find_package(CURL REQUIRED)

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
)

# Use the generated header files for the consumer application
set(radio-app-consumer_HEADERS
    "src/main/cpp/MyRadioHelper.h"
)

# Use the generated source files for the consumer application
set(radio-app-consumer_SOURCES
    "src/main/cpp/MyRadioHelper.cpp"
    "src/main/cpp/MyRadioConsumerApplication.cpp"
)

# Use the generated header files for the provider application
set(radio-app-provider_HEADERS
    ${GENERATED_HEADERS}
    "src/main/cpp/MyRadioProvider.h"
    "src/main/cpp/MyRadioHelper.h"
    "src/main/cpp/TrafficServiceBroadcastFilter.h"
    "src/main/cpp/GeocastBroadcastFilter.h"
)

# Use the generated source files for the provider application
set(radio-app-provider_SOURCES
    "src/main/cpp/MyRadioProvider.cpp"
    "src/main/cpp/MyRadioHelper.cpp"
    "src/main/cpp/MyRadioProviderApplication.cpp"
    "src/main/cpp/TrafficServiceBroadcastFilter.cpp"
    "src/main/cpp/GeocastBroadcastFilter.cpp"
)

# Put the generated code into a library and put the library in the bin directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_library(radio-app SHARED
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

target_link_libraries(radio-app
    Joynr::JoynrLib
)

# Compile the provider application (running in-process with cluster controller)
add_executable(radio-app-provider-cc
    ${radio-app-provider_HEADERS}
    ${radio-app-provider_SOURCES}
)

# Compile the consumer application (running in-process with cluster controller)
add_executable(radio-app-consumer-cc
    ${radio-app-consumer_HEADERS}
    ${radio-app-consumer_SOURCES}
)

# Link the provider application
target_link_libraries(radio-app-provider-cc
    radio-app
    Joynr::JoynrClusterControllerRuntime
)

# Link the consumer application
target_link_libraries(radio-app-consumer-cc
    radio-app
    Joynr::JoynrClusterControllerRuntime
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

# Include directories for WebSocket and UDS runtime are the same
if(TARGET Joynr::JoynrWsRuntime)
    get_target_property(
        LibJoynrRuntime_INCLUDE_DIRS
        Joynr::JoynrWsRuntime
        INTERFACE_INCLUDE_DIRECTORIES
    )
endif()
if(TARGET Joynr::JoynrUdsRuntime)
    get_target_property(
        LibJoynrRuntime_INCLUDE_DIRS
        Joynr::JoynrUdsRuntime
        INTERFACE_INCLUDE_DIRECTORIES
    )
endif()

get_target_property(
    LibJoynr_INCLUDE_DIRS
    Joynr::JoynrLib
    INTERFACE_INCLUDE_DIRECTORIES
)

# Use LibJoynr's definitions for compilation of joynr/Logger.h within radio-app
# to get log output to the same destinations (e.g. stdout).
get_target_property(
    LibJoynr_COMPILE_DEFINITIONS
    Joynr::JoynrLib
    INTERFACE_COMPILE_DEFINITIONS
)

# Provider objects are independent of cluster controller communication
add_library(radio-app-provider-common OBJECT
    ${radio-app-provider_HEADERS}
    ${radio-app-provider_SOURCES}
)

target_include_directories(radio-app-provider-common
    PUBLIC ${LibJoynr_INCLUDE_DIRS}
    PUBLIC ${LibJoynrRuntime_INCLUDE_DIRS}
)

target_compile_definitions(radio-app-provider-common
    PUBLIC ${LibJoynr_COMPILE_DEFINITIONS}
)

# Consumer objects are independent of cluster controller communication
add_library(radio-app-consumer-common OBJECT
    ${radio-app-consumer_HEADERS}
    ${radio-app-consumer_SOURCES}
)

target_include_directories(radio-app-consumer-common
    PUBLIC ${LibJoynr_INCLUDE_DIRS}
    PUBLIC ${LibJoynrRuntime_INCLUDE_DIRS}
)

target_compile_definitions(radio-app-consumer-common
    PUBLIC ${LibJoynr_COMPILE_DEFINITIONS}
)

function(create_apps TYPE RUNTIME)
    add_executable(radio-app-provider-${TYPE}
        $<TARGET_OBJECTS:radio-app-provider-common>)
    target_link_libraries(radio-app-provider-${TYPE}
        radio-app
        ${RUNTIME})

    add_executable(radio-app-consumer-${TYPE}
        $<TARGET_OBJECTS:radio-app-consumer-common>)
    target_link_libraries(radio-app-consumer-${TYPE}
        radio-app
        ${RUNTIME})
endfunction()

# Communicates via WebSockets with cluster controller
if(TARGET Joynr::JoynrWsRuntime)
    create_apps(ws Joynr::JoynrWsRuntime)
    install(TARGETS
                radio-app-provider-ws
                radio-app-consumer-ws
            RUNTIME DESTINATION bin COMPONENT bin
    )
endif()

# Communicates via UNIX Domain Sockets with cluster controller
if(TARGET Joynr::JoynrUdsRuntime)
    create_apps(uds Joynr::JoynrUdsRuntime)
    install(TARGETS
                radio-app-provider-uds
                radio-app-consumer-uds
            RUNTIME DESTINATION bin COMPONENT bin
    )
endif()

install(TARGETS
            radio-app-provider-cc
            radio-app-consumer-cc
        RUNTIME DESTINATION bin COMPONENT bin
)

AddClangFormat(radio-app-provider-common)
AddClangFormat(radio-app-consumer-common)
AddClangFormat(radio-app-provider-cc)
AddClangFormat(radio-app-consumer-cc)

# Set Visual Studio to do parallel builds
IF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
        set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVCC_AUTO_INCLUDE} /MP")

        # When compiling with DEBUG turn on runtime error checks (RTC1) and
        # Buffer Overflow detection (GS)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MSVCC_AUTO_INCLUDE} /RTC1 /GS /MP")
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")


# Visual Studio compiles to Debug and Release directories
IF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/cc.messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/cc.messaging.settings)
ELSE()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.messaging.settings)
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")

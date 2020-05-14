cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(robustness-test-env-1.0.0-SNAPSHOT)

# Compile to a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")
include(SetCppStandard)

##### cmake options #####################

option(
    GENERATE_SOURCES
    "Generate source files from Franca model?"
    OFF
)
message(STATUS "option GENERATE_SOURCES=" ${GENERATE_SOURCES})

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 1.8.10 REQUIRED)

if(${GENERATE_SOURCES})
    find_package(JoynrGenerator ${Joynr_VERSION} EXACT REQUIRED)
    message(STATUS "variable JoynrGenerator_FOUND=${JoynrGenerator_FOUND}")
    message(STATUS "variable JoynrGenerator_JAR=${JoynrGenerator_JAR}")
    JoynrGenerator(
        OUTPUTPATH ${CMAKE_CURRENT_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp
        MODELPATH ${CMAKE_CURRENT_SOURCE_DIR}/../robustness-test-base/src/main/model/RobustnessTest.fidl
        GENERATION_LANGUAGE "cpp"
    )
endif(${GENERATE_SOURCES})

include(AddClangFormat)

######## Add Boost ########

find_package(Boost 1.58.0 REQUIRED
    COMPONENTS
    system
    filesystem
    program_options
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp/include)

file(GLOB_RECURSE GENERATED_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp/include/*.h"
)

file(GLOB_RECURSE GENERATED_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp/*.cpp"
)

## CONSUMER
set(consumer_SOURCES
    ${GENERATED_SOURCES}
    "src/main/cpp/ConsumerMain.cpp"
    "src/main/cpp/ConsumerTest.cpp"
    "src/main/cpp/ConsumerProxy.cpp"
)

set(consumer_HEADERS
    ${GENERATED_HEADERS}
    "src/main/cpp/ConsumerTest.h"
)

add_executable(consumer-app
    ${consumer_HEADERS}
    ${consumer_SOURCES}
)

target_include_directories(consumer-app
    PRIVATE ${Joynr_LIB_COMMON_INCLUDE_DIRS}
    PRIVATE ${Boost_INCLUDE_DIRS}
)

target_link_libraries(consumer-app
    ${Joynr_LIB_WS_LIBRARIES}
    ${Boost_LIBRARIES}
)

## PROVIDER
set(provider_SOURCES
    ${GENERATED_SOURCES}
    "src/main/cpp/ProviderMain.cpp"
    "src/main/cpp/ProviderTest.cpp"
    "src/main/cpp/ProviderCheck.cpp"
)

set(provider_HEADERS
    ${GENERATED_HEADERS}
    "src/main/cpp/ProviderTest.h"
    "src/main/cpp/ProviderCheck.h"
)

add_executable(provider-app
    ${provider_HEADERS}
    ${provider_SOURCES}
)

target_include_directories(provider-app
    PRIVATE ${Joynr_LIB_COMMON_INCLUDE_DIRS}
    PRIVATE ${Boost_INCLUDE_DIRS}
)

target_link_libraries(provider-app
    ${Joynr_LIB_WS_LIBRARIES}
    ${Boost_LIBRARIES}
)

## INSTALL
file(
    COPY
    ${Joynr_RESOURCES_DIR}
    ${Joynr_EXECUTABLES}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

install(TARGETS
    consumer-app
    provider-app
    RUNTIME DESTINATION bin COMPONENT bin
)

AddClangFormat(consumer-app)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cpp-resources/provider-app.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/provider-app.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cpp-resources/consumer-app.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/consumer-app.settings)

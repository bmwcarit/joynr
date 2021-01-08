cmake_minimum_required(VERSION 3.10)

######## Project settings ########
project(jsit VERSION 1.15.8 LANGUAGES CXX)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/SetCppStandard.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/CMake")

option(
    GENERATE_SOURCES
    "Generate source files from Franca model?"
    OFF
)
message(STATUS "option GENERATE_SOURCES=" ${GENERATE_SOURCES})

option(
    USE_DUMMY_KEYCHAIN
    "Use a dummy keychain implementation?"
    ON
)
message(STATUS "option USE_DUMMY_KEYCHAIN=" ${USE_DUMMY_KEYCHAIN})
option(
    USE_REAL_KEYCHAIN
    "Use the real keychain implementation?"
    OFF
)

##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 1.15.8 REQUIRED HINTS "${CMAKE_BINARY_DIR}/../joynr/bin")
message(STATUS "joynr variable Joynr_VERSION=${Joynr_VERSION}")
message(STATUS "joynr variable Joynr_EXECUTABLES=${Joynr_EXECUTABLES}")
message(STATUS "joynr variable Joynr_RESOURCES_DIR=${Joynr_RESOURCES_DIR}")

if(${USE_DUMMY_KEYCHAIN} AND ${USE_REAL_KEYCHAIN})
    message(FATAL_ERROR "You need to set either USE_DUMMY_KEYCHAIN or USE_REAL_KEYCHAIN, but not both!")
endif()

set(KEYCHAIN_TARGET "")

if(${USE_DUMMY_KEYCHAIN})
    find_package(dummyKeychain 0.0.1 REQUIRED)
    message(STATUS "dummyKeychain VERSION=${dummyKeychain_VERSION}")
    set(KEYCHAIN_TARGET dummyKeychain)
    add_definitions(-DDUMMY_KEYCHAIN)
endif()
if(${USE_REAL_KEYCHAIN})
    find_package(CertManager REQUIRED)
    message(STATUS "CertManager: Using real keychain")
    set(KEYCHAIN_TARGET CertManager::certmanager)
    add_definitions(-DREAL_KEYCHAIN)
endif(${USE_REAL_KEYCHAIN})

if(${GENERATE_SOURCES})
    find_package(JoynrGenerator ${Joynr_VERSION} EXACT REQUIRED)
    message(STATUS "variable JoynrGenerator_FOUND=${JoynrGenerator_FOUND}")
    message(STATUS "variable JoynrGenerator_JAR=${JoynrGenerator_JAR}")
    JoynrGenerator(
        OUTPUTPATH ${CMAKE_SOURCE_DIR}/src/main/generated-sources
        MODELPATH ${CMAKE_SOURCE_DIR}/../src/main/model/SystemIntegrationTest.fidl
        GENERATION_LANGUAGE "cpp"
    )
endif(${GENERATE_SOURCES})

######## Add Boost ########

find_package(Boost 1.58.0 REQUIRED
    COMPONENTS
    system
    filesystem
    program_options
)
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})

##### Helper variables for using generated code ###########
file(
    GLOB_RECURSE
    GENERATED_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-sources/include/*.h"
)

file(
    GLOB_RECURSE
    GENERATED_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/main/generated-sources/*.cpp"
)

set(
    GENERATED_INCLUDE_DIRS
    "src/main/generated-sources/include"
)

# Put the Joynr_LIB_COMMON_INCLUDE_DIRS last so that it is possible to override types
# defined in libjoynr
include_directories(
    ${GENERATED_INCLUDE_DIRS}
)

set(
    common_HEADERS
    "src/main/cpp/SitUtil.h"
)

set(
    common_SOURCES
    "src/main/cpp/SitUtil.cpp"
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
    "src/main/cpp/SystemIntegrationTestProvider.h"
)

# Provider sources
set(
    jsit-provider_SOURCES
    "src/main/cpp/SystemIntegrationTestProvider.cpp"
    "src/main/cpp/ProviderApplication.cpp"
)

# copy joynr resources and settings
if(IS_DIRECTORY "${Joynr_RESOURCES_DIR}")
    file(
        COPY "${Joynr_RESOURCES_DIR}"
        DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endif()

# copy systemintegrationtest resources and settings
file(
    COPY "src/main/resources"
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Needed libraries
set(jsit-tests_LIBRARIES
    ${Boost_LIBRARIES}
    ${KEYCHAIN_TARGET}
)

# Compile the provider application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(
    jsit-provider-ws
    ${jsit-provider_HEADERS}
    ${jsit-provider_SOURCES}
    ${common_HEADERS}
    ${common_SOURCES}
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

# Link the provider application
target_link_libraries(
    jsit-provider-ws
    ${jsit-tests_LIBRARIES}
    Joynr::JoynrWsRuntime
)

# Compile the consumer application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(
    jsit-consumer-ws
    ${jsit-consumer_HEADERS}
    ${jsit-consumer_SOURCES}
    ${common_HEADERS}
    ${common_SOURCES}
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

# Link the consumer application
target_link_libraries(
    jsit-consumer-ws
    ${jsit-tests_LIBRARIES}
    Joynr::JoynrWsRuntime
)

install(
    TARGETS
    jsit-provider-ws
    jsit-consumer-ws
    RUNTIME DESTINATION bin
)

include(AddClangFormat)
if(ENABLE_CLANG_FORMATTER)
    AddClangFormat(jsit-consumer-ws)
    AddClangFormat(jsit-provider-ws)
endif(ENABLE_CLANG_FORMATTER)

if(NOT INSTALL_TEST_DIR)
    set(INSTALL_TEST_DIR "${CMAKE_INSTALL_BINDIR}" CACHE PATH "Installation sub directory for tests")
endif(NOT INSTALL_TEST_DIR)

install(
    FILES
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-consumer.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-provider.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-clustercontroller.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-consumer-ssl.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-provider-ssl.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-clustercontroller-ssl.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-provider-cc1-ssl.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-consumer-cc2-ssl.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-clustercontroller-cc1-ssl.settings
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/systemintegrationtest-clustercontroller-cc2-ssl.settings
    DESTINATION ${INSTALL_TEST_DIR}/resources
)

install(
    FILES
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/acl-lite/local-domain-access-store.json
    DESTINATION ${INSTALL_TEST_DIR}/resources/acl-lite/
)

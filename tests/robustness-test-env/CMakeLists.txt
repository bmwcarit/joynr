##
cmake_minimum_required(VERSION 3.1.0)
set (CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

project(prov-cons-demoapp)

set(PROV-CONS-DEMOAPP_MAJOR_VERSION 0)
set(PROV-CONS-DEMOAPP_MINOR_VERSION 1)
set(PROV-CONS-DEMOAPP_PATCH_VERSION 0)
set(PROV-CONS-DEMOAPP_VERSION
  ${PROV-CONS-DEMOAPP_MAJOR_VERSION}.${PROV-CONS-DEMOAPP_MINOR_VERSION}.${PROV-CONS-DEMOAPP_PATCH_VERSION})



set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

## PACKAGES
find_package(Joynr 0.33.0 REQUIRED)

if(${GENERATE_SOURCES})
    find_package(JoynrGenerator ${Joynr_VERSION} EXACT REQUIRED)
    message(STATUS "variable JoynrGenerator_FOUND=${JoynrGenerator_FOUND}")
    message(STATUS "variable JoynrGenerator_JAR=${JoynrGenerator_JAR}")
    JoynrGenerator(
        OUTPUTPATH ${CMAKE_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp
        MODELPATH ${CMAKE_SOURCE_DIR}/../robustness-test-base/src/main/model/RobustnessTest.fidl
        GENERATION_LANGUAGE "cpp"
    )
endif(${GENERATE_SOURCES})

find_package(Boost 1.58.0 REQUIRED
    COMPONENTS
    system
    filesystem
    program_options
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp/include)

file(GLOB_RECURSE GENERATED_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp/include/*.h"
)

file(GLOB_RECURSE GENERATED_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/../robustness-test-base/src/main/generated-cpp/*.cpp"
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
    provider-app
    RUNTIME DESTINATION bin COMPONENT bin
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cpp-resources/provider-app.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/provider-app.settings)


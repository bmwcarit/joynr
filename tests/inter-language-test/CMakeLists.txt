cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(inter-language-test-0.18.2-SNAPSHOT)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")
include(SetCppStandard)
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
find_package(Joynr 0.18.2 REQUIRED)
message(STATUS "joynr variable Joynr_LIB_COMMON_INCLUDE_DIRS=${Joynr_LIB_COMMON_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_INPROCESS_INCLUDE_DIRS=${Joynr_LIB_INPROCESS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_DBUS_INCLUDE_DIRS=${Joynr_LIB_DBUS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_WS_INCLUDE_DIRS=${Joynr_LIB_WS_INCLUDE_DIRS}")
message(STATUS "joynr variable Joynr_LIB_COMMON_LIBRARIES=${Joynr_LIB_COMMON_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_INPROCESS_LIBRARIES=${Joynr_LIB_INPROCESS_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_DBUS_LIBRARIES=${Joynr_LIB_DBUS_LIBRARIES}")
message(STATUS "joynr variable Joynr_LIB_WS_LIBRARIES=${Joynr_LIB_WS_LIBRARIES}")
message(STATUS "joynr variable Joynr_EXECUTABLES=${Joynr_EXECUTABLES}")
message(STATUS "joynr variable Joynr_RESOURCES_DIR=${Joynr_RESOURCES_DIR}")
message(STATUS "joynr variable USE_DBUS_COMMONAPI_COMMUNICATION=${USE_DBUS_COMMONAPI_COMMUNICATION}")

######## Add Boost ########

find_package(Boost 1.56.0 REQUIRED
    COMPONENTS
    system
    filesystem
)
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})

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
    ${Joynr_LIB_COMMON_INCLUDE_DIRS}
)

# Use the generated header files for the consumer application
set(ilt-consumer_HEADERS
    "src/main/cpp/IltHelper.h"
    "src/main/cpp/IltUtil.h"
)

# Use the generated source files for the consumer application
set(ilt-consumer_SOURCES
    "src/main/cpp/IltHelper.cpp"
    "src/main/cpp/IltUtil.cpp"
    "src/main/cpp/IltConsumerApplication.cpp"
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
    "src/main/cpp/IltUtil.cpp"
    "src/main/cpp/IltProviderApplication.cpp"
    "src/main/cpp/IltStringBroadcastFilter.cpp"
)

# Link with the joynr libraries
set(ilt_LIBRARIES
    ${Joynr_LIB_COMMON_LIBRARIES}
    ${Boost_LIBRARIES}
)

# Put the generated code into a library and put the library in the bin directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    find_package(PkgConfig)
    pkg_check_modules(DBUS1 REQUIRED dbus-1)
    message(STATUS "variable DBUS_INCLUDE_DIRS=" ${DBUS1_INCLUDE_DIRS})
    message(STATUS "variable DBUS1_LIBRARIES=" ${DBUS1_LIBRARIES})
    message(STATUS "variable DBUS1_LIBRARY_DIRS=" ${DBUS1_LIBRARY_DIRS})
    pkg_check_modules(COMMONAPI REQUIRED CommonAPI=2.1.4)
    message(STATUS "variable COMMONAPI_INCLUDE_DIRS=" ${COMMONAPI_INCLUDE_DIRS})
    message(STATUS "variable COMMONAPI_LIBRARIES=" ${COMMONAPI_LIBRARIES})
    message(STATUS "variable COMMONAPI_VERSION=" ${COMMONAPI_VERSION})
    pkg_check_modules(COMMONAPI_DBUS REQUIRED CommonAPI-DBus=2.1.4)
    message(STATUS "variable COMMONAPI_DBUS_INCLUDE_DIRS=" ${COMMONAPI_DBUS_INCLUDE_DIRS})
    message(STATUS "variable COMMONAPI_DBUS_LIBRARIES=" ${COMMONAPI_DBUS_LIBRARIES})
    message(STATUS "variable COMMONAPI_DBUS_VERSION=" ${COMMONAPI_DBUS_VERSION})

    link_directories(${DBUS1_LIBRARY_DIRS} ${COMMONAPI_LIBRARY_DIRS} ${COMMONAPI_DBUS_LIBRARY_DIRS})
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

add_library(ilt SHARED
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

target_link_libraries(ilt
    ${ilt_LIBRARIES}
)

# Compile the provider application (running in-process with cluster controller)
add_executable(ilt-provider-cc
    ${ilt-provider_HEADERS}
    ${ilt-provider_SOURCES}
)

# Compile the consumer application (running in-process with cluster controller)
add_executable(ilt-consumer-cc
    ${ilt-consumer_HEADERS}
    ${ilt-consumer_SOURCES}
)

# Link the provider application
target_link_libraries(ilt-provider-cc
    ilt
    ${ilt_LIBRARIES}
    ${Joynr_LIB_INPROCESS_LIBRARIES}
)

# Link the consumer application
target_link_libraries(ilt-consumer-cc
    ilt
    ${ilt_LIBRARIES}
    ${Joynr_LIB_INPROCESS_LIBRARIES}
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
add_executable(ilt-provider-ws
    ${ilt-provider_HEADERS}
    ${ilt-provider_SOURCES}
)

# Link the provider application
target_link_libraries(ilt-provider-ws
    ilt
    ${ilt_LIBRARIES}
    ${Joynr_LIB_WS_LIBRARIES}
)

# Compile the consumer application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(ilt-consumer-ws
    ${ilt-consumer_HEADERS}
    ${ilt-consumer_SOURCES}
)

# Link the consumer application
target_link_libraries(ilt-consumer-ws
    ilt
    ${ilt_LIBRARIES}
    ${Joynr_LIB_WS_LIBRARIES}
)

install(TARGETS
            ilt-provider-ws
            ilt-consumer-ws
            ilt-provider-cc
            ilt-consumer-cc
        RUNTIME DESTINATION bin COMPONENT bin
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    # Compile the provider application (requires: Dbus and running cluster controller)
    # Communicates via Dbus with cluster controller
    add_executable(ilt-provider-dbus
        ${ilt-provider_HEADERS}
        ${ilt-provider_SOURCES}
    )

    # Link the provider application
    target_link_libraries(ilt-provider-dbus
        ilt
        ${ilt_LIBRARIES}
        ${Joynr_LIB_DBUS_LIBRARIES}
    )

    # Compile the consumer application (requires: Dbus and running cluster controller)
    # Communicates via Dbus with cluster controller
    add_executable(ilt-consumer-dbus
        ${ilt-consumer_HEADERS}
        ${ilt-consumer_SOURCES}
    )

    # Link the consumer application
    target_link_libraries(ilt-consumer-dbus
        ilt
        ${ilt_LIBRARIES}
        ${Joynr_LIB_DBUS_LIBRARIES}
    )

    install(TARGETS
                ilt-provider-dbus
                ilt-consumer-dbus
            RUNTIME DESTINATION bin COMPONENT bin
    )

    AddClangFormat(ilt-provider-dbus)
    AddClangFormat(ilt-consumer-dbus)

endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

AddClangFormat(ilt-provider-cc)
AddClangFormat(ilt-consumer-cc)
AddClangFormat(ilt-provider-ws)
AddClangFormat(ilt-consumer-ws)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/ilt-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/ilt-provider.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/ilt-provider.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/ilt-provider.dbus.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/ilt-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/ilt-consumer.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/ilt-consumer.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/ilt-consumer.dbus.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.messaging.settings)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.dbus.settings)

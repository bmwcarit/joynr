cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(radio-app-0.13.0)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
add_definitions("-std=gnu++0x")

set(
    JOYNR_SERVER
    "localhost:8080"
    CACHE STRING "joynr infrastructure server (BounceProxy, CapDir, ChannelUrlDir) to use in settings files."
)

message(STATUS "option JOYNR_SERVER=${JOYNR_SERVER}")

option(
    ENABLE_CLANG_FORMATTER
    "Use clang-formatter to format source code?"
    ON
)
message(STATUS "option ENABLE_CLANG_FORMATTER=" ${ENABLE_CLANG_FORMATTER})


##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 0.13.0 REQUIRED)
message(STATUS "joynr variable JOYNR_INCLUDE_DIRS=${JOYNR_INCLUDE_DIRS}")
message(STATUS "joynr variable JOYNR_LIB_DBUS_INCLUDE_DIRS=${JOYNR_LIB_DBUS_INCLUDE_DIRS}")
message(STATUS "joynr variable JOYNR_LIB_WS_INCLUDE_DIRS=${JOYNR_LIB_WS_INCLUDE_DIRS}")
message(STATUS "joynr variable JOYNR_LIBRARIES=${JOYNR_LIBRARIES}")
message(STATUS "joynr variable JOYNR_LIB_DBUS_LIBRARIES=${JOYNR_LIB_DBUS_LIBRARIES}")
message(STATUS "joynr variable JOYNR_LIB_WS_LIBRARIES=${JOYNR_LIB_WS_LIBRARIES}")
message(STATUS "joynr variable JOYNR_EXECUTABLES=${JOYNR_EXECUTABLES}")
message(STATUS "joynr variable JOYNR_RESOURCES_DIR=${JOYNR_RESOURCES_DIR}")
message(STATUS "joynr variable USE_DBUS_COMMONAPI_COMMUNICATION=${USE_DBUS_COMMONAPI_COMMUNICATION}")

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

# Put the JOYNR_INCLUDE_DIRS last so that it is possible to override types
# defined in libjoyn
include_directories(
    ${GENERATED_INCLUDE_DIRS}
    ${JOYNR_INCLUDE_DIRS}
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

# Link with the joynr libraries
set(radio-app_LIBRARIES
    ${JOYNR_LIBRARIES}
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

add_library(radio-app SHARED
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

target_link_libraries(radio-app ${JOYNR_LIBRARIES})

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
    ${radio-app_LIBRARIES}
)

# Link the consumer application
target_link_libraries(radio-app-consumer-cc
    radio-app
    ${radio-app_LIBRARIES}
)

# copy joynr resources and settings
file(
    COPY ${JOYNR_RESOURCES_DIR}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)



# copy joynr executables
file(
    COPY ${JOYNR_EXECUTABLES}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Compile the provider application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(radio-app-provider-ws
    ${radio-app-provider_HEADERS}
    ${radio-app-provider_SOURCES}
)

# Link the provider application
target_link_libraries(radio-app-provider-ws
    radio-app
    ${JOYNR_LIB_WS_LIBRARIES}
)

# Compile the consumer application (requires: running cluster controller)
# Communicates via WebSockets with cluster controller
add_executable(radio-app-consumer-ws
    ${radio-app-consumer_HEADERS}
    ${radio-app-consumer_SOURCES}
)

# Link the consumer application
target_link_libraries(radio-app-consumer-ws
    radio-app
    ${JOYNR_LIB_WS_LIBRARIES}
)

install(TARGETS
            radio-app-provider-ws
            radio-app-consumer-ws
            radio-app-provider-cc
            radio-app-consumer-cc
        RUNTIME DESTINATION bin COMPONENT bin
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    # Compile the provider application (requires: Dbus and running cluster controller)
    # Communicates via Dbus with cluster controller
    add_executable(radio-app-provider-dbus
        ${radio-app-provider_HEADERS}
        ${radio-app-provider_SOURCES}
    )

    # Link the provider application
    target_link_libraries(radio-app-provider-dbus
        radio-app
        ${JOYNR_LIB_DBUS_LIBRARIES}
    )

    # Compile the consumer application (requires: Dbus and running cluster controller)
    # Communicates via Dbus with cluster controller
    add_executable(radio-app-consumer-dbus
        ${radio-app-consumer_HEADERS}
        ${radio-app-consumer_SOURCES}
    )

    # Link the consumer application
    target_link_libraries(radio-app-consumer-dbus
        radio-app
        ${JOYNR_LIB_DBUS_LIBRARIES}
    )

    install(TARGETS
                radio-app-provider-dbus
                radio-app-consumer-dbus
            RUNTIME DESTINATION bin COMPONENT bin
    )

endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

if(${ENABLE_CLANG_FORMATTER})
    set(COMMENT "running C++ code formatter: clang-format")
    set(COMMENT "${COMMENT}\nworking directory: ${CMAKE_CURRENT_SOURCE_DIR}")
    set(COMMENT "${COMMENT}\nprocessing header files:\n${radio-app-provider_HEADERS}")
    set(COMMENT "${COMMENT}\nprocessing source files:\n${radio-app-provider_SOURCES}")

    add_custom_command(
        TARGET radio-app-provider-cc
        PRE_BUILD
        COMMAND clang-format -i
            ${radio-app-provider_HEADERS}
            ${radio-app-provider_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "${COMMENT}"
        VERBATIM
    )

    set(COMMENT "running C++ code formatter: clang-format")
    set(COMMENT "${COMMENT}\nworking directory: ${CMAKE_CURRENT_SOURCE_DIR}")
    set(COMMENT "${COMMENT}\nprocessing header files:\n${radio-app-consumer_HEADERS}")
    set(COMMENT "${COMMENT}\nprocessing source files:\n${radio-app-consumer_SOURCES}")

    add_custom_command(
        TARGET radio-app-consumer-cc
        PRE_BUILD
        COMMAND clang-format -i
            ${radio-app-consumer_HEADERS}
            ${radio-app-consumer_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "${COMMENT}"
        VERBATIM
    )
endif(${ENABLE_CLANG_FORMATTER})

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
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/radio-app-provider.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug//resources/radio-app-consumer.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/cc.messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug//resources/cc.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-provider.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release//resources/radio-app-consumer.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/cc.messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release//resources/cc.dbus.settings)
ELSE()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-provider.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-consumer.dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/cc.dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/cc.dbus.settings)
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")

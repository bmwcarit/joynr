cmake_minimum_required(VERSION 2.8.9)

######## Project settings ########
project(radio-app-1.3.0)

# Compile to a bin directory current directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
add_definitions("-std=gnu++0x")

set(
    JOYNR_SERVER
    "localhost:8080"
    CACHE STRING "JOYn infrastructure server (BounceProxy, CapDir, ChannelUrlDir) to use in settings files."
)

message(STATUS "option JOYNR_SERVER=${JOYNR_SERVER}")
##### Start of build ####################

# Pull in the Joynr configuration
find_package(Joynr 0.6.0 REQUIRED)
message(STATUS "JOYn variable JOYNR_INCLUDE_DIRS=${JOYNR_INCLUDE_DIRS}")
message(STATUS "JOYn variable JOYNR_LIBRARIES=${JOYNR_LIBRARIES}")

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
)

# Use the generated source files for the provider application
set(radio-app-provider_SOURCES
    "src/main/cpp/MyRadioProvider.cpp"
    "src/main/cpp/MyRadioHelper.cpp"
    "src/main/cpp/MyRadioProviderApplication.cpp"
)

# Link with the JOYn libraries
set(radio-app_LIBRARIES
    ${JOYNR_LIBRARIES}
)

# Put the generated code into a library and put the library in the bin directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_library(radio-app STATIC
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)

# Compile the provider application
add_executable(radio-app-provider
    ${radio-app-provider_HEADERS}
    ${radio-app-provider_SOURCES}
)

# Compile the consumer application
add_executable(radio-app-consumer
    ${radio-app-consumer_HEADERS}
    ${radio-app-consumer_SOURCES}
)

# Link the provider application
target_link_libraries(radio-app-provider
    radio-app
    ${radio-app_LIBRARIES}
)

# Link the consumer application
target_link_libraries(radio-app-consumer
    radio-app
    ${radio-app_LIBRARIES}
)

# function to copy joyn executables to current binary directory
function(copy_JOYNR_EXECUTABLES target_name)
    foreach(exec ${JOYNR_EXECUTABLE})
        GET_FILENAME_COMPONENT(execName ${exec} NAME)
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${exec}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${execName}
        )
    endforeach(exec)
endfunction(copy_JOYNR_EXECUTABLES)

# copy joyn executables
copy_JOYNR_EXECUTABLES(radio-app-consumer)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    # Compile the provider application
    add_executable(radio-app-provider_ClusterController
        ${radio-app-provider_HEADERS}
        ${radio-app-provider_SOURCES}
    )

    # Link the provider application
    target_link_libraries(radio-app-provider_ClusterController
        radio-app
        ${JOYNR_CLUSTER_CONTROLLER_LIBRARIES}
    )

    # Compile the consumer application
    add_executable(radio-app-consumer_ClusterController
        ${radio-app-consumer_HEADERS}
        ${radio-app-consumer_SOURCES}
    )

    # Link the consumer application
    target_link_libraries(radio-app-consumer_ClusterController
        radio-app
        ${JOYNR_CLUSTER_CONTROLLER_LIBRARIES}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

MESSAGE("CMAKE_CURRENT_SOURCE_DIR:" ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE("CMAKE_RUNTIME_OUTPUT_DIRECTORY:" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

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
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.libjoyn.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/radio-app-provider.libjoyn.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.libjoyn.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug//resources/radio-app-consumer.libjoyn.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.libjoyn.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-provider.libjoyn.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.libjoyn.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release//resources/radio-app-consumer.libjoyn.settings)
ELSE()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-provider.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-provider.libjoyn.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-provider.libjoyn.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-consumer.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/radio-app-consumer.libjoyn.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/radio-app-consumer.libjoyn.settings)
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")

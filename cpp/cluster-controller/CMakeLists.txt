message(STATUS "#############################################")
message(STATUS "Configuring JoynrClusterController...")
message(STATUS "#############################################\n")

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrLib_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)

### Add mosquitto mqtt ###########################################################
if(USE_PLATFORM_MOSQUITTO)
    find_package(mosquitto REQUIRED)

    message(STATUS "variable MOSQUITTO_INCLUDE_DIRS=${MOSQUITTO_INCLUDE_DIRS}")
    set(MOSQUITTO_INCLUDE_DIRS ${MOSQUITTO_INCLUDE_DIRS} PARENT_SCOPE)
    get_filename_component(MOSQUITTO_BINARY_DIR ${MOSQUITTO_LIBRARIES} DIRECTORY)
    message(STATUS "variable MOSQUITTO_BINARY_DIR=${MOSQUITTO_BINARY_DIR}")
    message(STATUS "variable MOSQUITTO_LIBRARIES=${MOSQUITTO_LIBRARIES}")
else(USE_PLATFORM_MOSQUITTO)
    message(STATUS "############ Download: mosquitto ############")
    set(MOSQUITTO_SOURCE_URL "https://git.eclipse.org/r/mosquitto/org.eclipse.mosquitto.git")
    set(MOSQUITTO_SOURCE_TYPE "GIT_REPOSITORY")
    set(MOSQUITTO_VERSION 1.4.7)
    set(MOSQUITTO_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}mosquittopp${CMAKE_SHARED_LIBRARY_SUFFIX})

    AddExternalProject(
        mosquitto
        ${MOSQUITTO_SOURCE_TYPE} "${MOSQUITTO_SOURCE_URL}"
        GIT_TAG v${MOSQUITTO_VERSION}
        TIMEOUT 10
        BUILD_BYPRODUCTS("${CMAKE_BINARY_DIR}/ThirdParty/src/mosquitto-build/lib/cpp/${MOSQUITTO_LIBRARY_NAME}")
    )

    ExternalProject_Get_Property(mosquitto SOURCE_DIR)

    # The directory does not exist when the import targets are checked. As a workaround
    # we need to create it here. Otherwise the initial cmake call will fail.
    file(MAKE_DIRECTORY ${SOURCE_DIR}/lib/cpp)

    set(MOSQUITTO_INCLUDE_DIRS ${SOURCE_DIR}/lib/cpp ${SOURCE_DIR}/lib)
    set(MOSQUITTO_INCLUDE_DIRS ${SOURCE_DIR}/lib/cpp ${SOURCE_DIR}/lib PARENT_SCOPE)
    message(STATUS "variable MOSQUITTO_INCLUDE_DIRS=${MOSQUITTO_INCLUDE_DIRS}")

    ExternalProject_Get_Property(mosquitto BINARY_DIR)
    set(MOSQUITTO_BINARY_DIR ${BINARY_DIR}/lib/cpp)
    message(STATUS "variable MOSQUITTO_BINARY_DIR=${MOSQUITTO_BINARY_DIR}")
    set(MOSQUITTO_LIBRARIES ${MOSQUITTO_BINARY_DIR}/${MOSQUITTO_LIBRARY_NAME})
    message(STATUS "variable MOSQUITTO_LIBRARIES=${MOSQUITTO_LIBRARIES}")
endif(USE_PLATFORM_MOSQUITTO)
include(CheckMosquittoImportTargets)

set(
    JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(
    JoynrClusterController_INCLUDE_DIRECTORIES
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrLib_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrClusterController_SYS_INCLUDE_DIRECTORIES
    ${CURL_INCLUDE_DIRS}
    ${MOSQUITTO_INCLUDE_DIRS}
)

set(
    JoynrClusterController_TARGET_LIBRARIES
    Joynr
    JoynrCommon
    cURL::curl
)

set(
    JoynrClusterController_EXPORT_HEADER
    "include/joynr/JoynrClusterControllerExport.h"
)


file(
    GLOB JoynrClusterController_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

file(
    GLOB_RECURSE JoynrClusterController_PRIVATE_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "access-control/*.h"
    "capabilities-client/*.h"
    "http-communication-manager/*.h"
    "httpnetworking/*.h"
    "ClusterControllerDirectories.h"
    "ICommunicationManager.h"
    "messaging/*.h"
    "messaging/in-process/*.h"
    "messaging/joynr-messaging/*.h"
    "mqtt/*.h"
    "ClusterControllerSettings.cpp"
)

set(
    JoynrClusterController_HEADERS
    ${JoynrClusterController_PUBLIC_HEADERS}
    ${JoynrClusterController_PRIVATE_HEADERS}
)


file(
    GLOB JoynrClusterController_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "access-control/*.cpp"
    "capabilities-client/*.cpp"
    "http-communication-manager/*.cpp"
    "httpnetworking/*.cpp"
    "messaging/*.cpp"
    "messaging/in-process/*.cpp"
    "messaging/joynr-messaging/*.cpp"
    "mqtt/*.cpp"
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    file(
        GLOB_RECURSE JoynrClusterController_DBUS_PRIVATE_HEADERS
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "messaging/dbus/*.h"
    )

    set(
        JoynrClusterController_HEADERS
        ${JoynrClusterController_HEADERS}
        ${JoynrClusterController_DBUS_PRIVATE_HEADERS}
    )

    file(
        GLOB_RECURSE JoynrClusterController_DBUS_SOURCES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "messaging/dbus/*.cpp"
    )

    set(
        JoynrClusterController_SOURCES
        ${JoynrClusterController_SOURCES}
        ${JoynrClusterController_DBUS_SOURCES}
    )

    list(
        APPEND JoynrClusterController_TARGET_LIBRARIES
        ${DBUS1_LIBRARIES}
        ${COMMONAPI_LIBRARIES}
        ${COMMONAPI_DBUS_LIBRARIES}
    )

    get_target_property(common_api_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrClusterController_INCLUDE_DIRECTORIES
        ${common_api_PUBLIC_INCLUDE_DIRECTORY}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})


add_library(
    JoynrClusterController
    # library type is set via BUILD_SHARED_LIBS
    ${JoynrClusterController_HEADERS}
    ${JoynrClusterController_SOURCES}
)

target_include_directories(
    JoynrClusterController
    PRIVATE ${JoynrClusterController_INCLUDE_DIRECTORIES}
)

target_include_directories(
    JoynrClusterController
    SYSTEM
    PRIVATE ${JoynrClusterController_SYS_INCLUDE_DIRECTORIES}
)

if(NOT USE_PLATFORM_MOSQUITTO)
    # make sure the external project mosquitto is built before this
    add_dependencies(JoynrClusterController mosquitto::mosquitto)
endif(NOT USE_PLATFORM_MOSQUITTO)

# manually add dependency to target
list(
    APPEND JoynrClusterController_TARGET_LIBRARIES
    mosquitto::mosquitto
)


AddClangFormat(JoynrClusterController)

AddIncludeWhatYouUse(JoynrClusterController)

AddClangTidy(JoynrClusterController)

set_target_properties(
    JoynrClusterController
    PROPERTIES
    SOVERSION "${JOYNR_SOVERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}"
)

include(GenerateJoynrExports)
GenerateJoynrExports(
    JoynrClusterController
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrClusterController_EXPORT_HEADER}"
)

target_link_libraries(JoynrClusterController ${JoynrClusterController_TARGET_LIBRARIES})

install(
    TARGETS JoynrClusterController
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

# Visual Studio compiles to Debug and Release directories
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-dbus.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-dbus.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-dbus.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-dbus.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings
        @ONLY
    )
else()
    # copy resources for build tree
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-dbus.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-dbus.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-clustercontroller.settings
        @ONLY
    )
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-dbus.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-clustercontroller.settings
        DESTINATION ${JOYNR_INSTALL_FULL_SYSCONFDIR}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

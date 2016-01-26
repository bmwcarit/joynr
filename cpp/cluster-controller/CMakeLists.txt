message(STATUS "#############################################")
message(STATUS "Configuring JoynrClusterController...")
message(STATUS "#############################################\n")

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrLib_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY JoynrJsonSerializer PUBLIC_INCLUDE_DIRECTORY)

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
    ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrClusterController_SYS_INCLUDE_DIRECTORIES
    ${jsmn_INCLUDE_DIR}
    ${Qt5Core_INCLUDE_DIRECTORY}
)

set(
    JoynrClusterController_TARGET_LIBRARIES
    Joynr
    JoynrCommon
    ${CURL_LIBRARIES}
    ${Qt5Sql_LIBRARIES}
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

include_directories(
        ${JoynrClusterController_INCLUDE_DIRECTORIES}
)

include_directories(
    SYSTEM
    ${JoynrClusterController_SYS_INCLUDE_DIRECTORIES}
)

add_library(
    JoynrClusterController
    SHARED
    ${JoynrClusterController_HEADERS}
    ${JoynrClusterController_SOURCES}
)

AddClangFormat(JoynrClusterController)

AddIncludeWhatYouUse(JoynrClusterController)

AddClangTidy(JoynrClusterController)

set_target_properties(
    JoynrClusterController
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}"
    AUTOMOC TRUE
)

generate_export_header(
    JoynrClusterController
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrClusterController_EXPORT_HEADER}"
)

target_link_libraries(JoynrClusterController ${JoynrClusterController_TARGET_LIBRARIES})

install(
    TARGETS JoynrClusterController
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${INSTALL_INCLUDE_DIR}"
    FILES_MATCHING PATTERN "*.h"
)

# Visual Studio compiles to Debug and Release directories
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings)
else()
    # copy resources for build tree
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-dbus.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-dbus.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings)
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-dbus.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
        DESTINATION ${INSTALL_RES_DIR}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

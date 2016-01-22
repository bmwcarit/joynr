message(STATUS "#############################################")
message(STATUS "Configuring runtimes...")
message(STATUS "#############################################\n")

get_target_property(jsonserializer_PUBLIC_INCLUDE_DIRECTORY jsonserializer PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(Joynr_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)

set(
    runtime_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
set(
    runtime_PUBLIC_INCLUDE_DIRECTORY
    "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
    PARENT_SCOPE
)

file(
    GLOB_RECURSE runtime_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

set(
    JoynrRuntime_INCLUDE_DIRECTORIES
    ${runtime_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${jsmn_INCLUDE_DIR}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
)

if(${BUILD_CLUSTER_CONTROLLER})
    get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
    set(
        JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES
        ${runtime_PUBLIC_INCLUDE_DIRECTORY}
        "../"
        ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
        ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
        ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    )

    set(
        JoynrClusterControllerRuntime_TARGET_LIBRARIES
        JoynrCommon
        Joynr
        JoynrClusterController
    )

    set(
        JoynrClusterControllerRuntime_EXPORT_HEADER
        "include/joynr/JoynrClusterControllerRuntimeExport.h"
    )

    file(
        GLOB JoynrClusterControllerRuntime_PRIVATE_HEADERS
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "*.h"
        "cluster-controller-runtime/*.h"
        "cluster-controller-runtime/websocket/*.h"
    )

    set(
        JoynrClusterControllerRuntime_HEADERS
        ${JoynrClusterControllerRuntime_PRIVATE_HEADERS}
        ${runtime_PUBLIC_HEADERS}
    )

    file(
        GLOB JoynrClusterControllerRuntime_SOURCES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "*.cpp"
        "cluster-controller-runtime/*.cpp"
        "cluster-controller-runtime/websocket/*.cpp"
    )

    include_directories(
        ${JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES}
    )

    include_directories(
        SYSTEM
        ${Qt5WebSockets_INCLUDE_DIRS}
        ${Qt5Core_INCLUDE_DIRS}
    )

    add_library(
        JoynrClusterControllerRuntime
        SHARED
        ${JoynrClusterControllerRuntime_HEADERS}
        ${JoynrClusterControllerRuntime_SOURCES}
    )

    AddClangFormat(JoynrClusterControllerRuntime)

    AddIncludeWhatYouUse(JoynrClusterControllerRuntime)

    AddClangTidy(JoynrClusterControllerRuntime)

    set_target_properties(
        JoynrClusterControllerRuntime
        PROPERTIES
        SOVERSION "${JOYNR_VERSION}"
        VERSION "${JOYNR_VERSION}"
        PUBLIC_INCLUDE_DIRECTORY "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
        AUTOMOC TRUE
    )

    generate_export_header(
        JoynrClusterControllerRuntime
        EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrClusterControllerRuntime_EXPORT_HEADER}"
    )

    target_link_libraries(
        JoynrClusterControllerRuntime
        ${JoynrClusterControllerRuntime_TARGET_LIBRARIES}
        ${Qt5WebSockets_LIBRARIES}
        ${Qt5Core_LIBRARIES}
    )

    install(
        TARGETS JoynrClusterControllerRuntime
        EXPORT JoynrTargets
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT library
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
        # this will flatten directory hierarchy of header files
        # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev
    )
endif(${BUILD_CLUSTER_CONTROLLER})


# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${INSTALL_INCLUDE_DIR}"
    FILES_MATCHING PATTERN "*.h"
)

set(
    JoynrRuntime_PRIVATE_HEADERS
    "libjoynr-runtime/JoynrRuntimeExecutor.h"
    "libjoynr-runtime/LibJoynrRuntime.h"
)

set(
    JoynrRuntime_SOURCES
    "libjoynr-runtime/JoynrRuntimeExecutor.cpp"
    "libjoynr-runtime/LibJoynrRuntime.cpp"
)

# library for applications
set(
    JoynrRuntime_EXPORT_HEADER
    "include/joynr/JoynrRuntimeExport.h"
)

#######################################################
# build libraries and executable for WebSocket
#######################################################

set(
    JoynrWsRuntime_PRIVATE_HEADERS
    "libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
    "libjoynr-runtime/websocket/JoynrWebSocketRuntimeExecutor.h"
)

set(
    JoynrWsRuntime_SOURCES
    "libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.cpp"
    "libjoynr-runtime/websocket/JoynrRuntime.cpp"
    "libjoynr-runtime/websocket/JoynrWebSocketRuntimeExecutor.cpp"
)

include_directories(
    ${JoynrRuntime_INCLUDE_DIRECTORIES}
    ${libwebsockets_INCLUDE_DIRS}
    ${jsonserializer_PUBLIC_INCLUDE_DIRECTORY}
)

add_library(
    JoynrWsRuntime
    SHARED
    ${JoynrRuntime_PRIVATE_HEADERS}
    ${JoynrRuntime_SOURCES}
    ${JoynrWsRuntime_PRIVATE_HEADERS}
    ${JoynrWsRuntime_SOURCES}
)

AddClangFormat(JoynrWsRuntime)

AddIncludeWhatYouUse(JoynrWsRuntime)

AddClangTidy(JoynrWsRuntime)

target_link_libraries(
    JoynrWsRuntime
    JoynrCommon
    Joynr
    jsonserializer
)

generate_export_header(
    JoynrWsRuntime
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrRuntime_EXPORT_HEADER}"
)

set_target_properties(
    JoynrWsRuntime
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
)

install(
    TARGETS JoynrWsRuntime
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT library
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
    # this will flatten directory hierarchy of header files
    # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${INSTALL_INCLUDE_DIR}"
    FILES_MATCHING PATTERN "*.h"
)

#######################################################
# build libraries and executable for dbus common api
#######################################################

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    set(
        JoynrDbusRuntime_PRIVATE_HEADERS
        "libjoynr-runtime/dbus/LibJoynrDbusRuntime.h"
        "libjoynr-runtime/dbus/JoynrDbusRuntimeExecutor.h"
    )

    set(
        JoynrDbusRuntime_SOURCES
        "libjoynr-runtime/dbus/LibJoynrDbusRuntime.cpp"
        "libjoynr-runtime/dbus/JoynrRuntime.cpp"
        "libjoynr-runtime/dbus/JoynrDbusRuntimeExecutor.cpp"
    )

    get_target_property(common_api_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrRuntime_INCLUDE_DIRECTORIES
        ${common_api_PUBLIC_INCLUDE_DIRECTORY}
    )

    include_directories(
        ${JoynrRuntime_INCLUDE_DIRECTORIES}
    )

    add_library(
        JoynrDbusRuntime
        SHARED
        ${JoynrRuntime_HEADERS}
        ${JoynrRuntime_SOURCES}
        ${JoynrDbusRuntime_PRIVATE_HEADERS}
        ${JoynrDbusRuntime_SOURCES}
    )

    AddClangFormat(JoynrDbusRuntime)

    AddIncludeWhatYouUse(JoynrDbusRuntime)

    AddClangTidy(JoynrDbusRuntime)

    target_link_libraries(
        JoynrDbusRuntime
        JoynrCommon
        Joynr
    )

    generate_export_header(
        JoynrDbusRuntime
        EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrRuntime_EXPORT_HEADER}"
    )

    set_target_properties(
        JoynrDbusRuntime
        PROPERTIES
        SOVERSION "${JOYNR_VERSION}"
        VERSION "${JOYNR_VERSION}"
        PUBLIC_INCLUDE_DIRECTORY "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
    )

    install(
        TARGETS JoynrDbusRuntime
        EXPORT JoynrTargets
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
        # this will flatten directory hierarchy of header files
        # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev
    )
    # so we install header files manually to preserve directory hierarchy
    install(
        DIRECTORY include/
        DESTINATION "${INSTALL_INCLUDE_DIR}"
        FILES_MATCHING PATTERN "*.h"
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})


if(${BUILD_CLUSTER_CONTROLLER})
    #executable to start the cluster controller
    set(
        cluster-controller_HEADERS
    )

    set(
        cluster-controller_SOURCES
        StartClusterController.cpp
    )

    add_executable(
        cluster-controller
        ${cluster-controller_HEADERS}
        ${cluster-controller_SOURCES}
    )

    AddClangFormat(cluster-controller)

    AddClangTidy(cluster-controller)

    if(${USE_DBUS_COMMONAPI_COMMUNICATION})
        target_link_libraries(cluster-controller
            ${DBUS1_LIBRARIES}
            ${COMMONAPI_DBUS_LIBRARIES}
            ${COMMONAPI_LIBRARIES}
        )
    endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

    target_link_libraries(cluster-controller
        JoynrClusterControllerRuntime
    )

    install(
        TARGETS cluster-controller
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
    )
endif(${BUILD_CLUSTER_CONTROLLER})

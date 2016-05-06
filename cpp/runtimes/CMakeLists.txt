message(STATUS "#############################################")
message(STATUS "Configuring runtimes...")
message(STATUS "#############################################\n")

get_target_property(JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY JoynrJsonSerializer PUBLIC_INCLUDE_DIRECTORY)
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
    ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
)

include(GenerateJoynrExports)

if(${BUILD_CLUSTER_CONTROLLER})
    get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
    set(
        JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES
        ${JoynrRuntime_INCLUDE_DIRECTORIES}
        ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
        ${MOSQUITTO_INCLUDE_DIRS}
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

    add_library(
        JoynrClusterControllerRuntime
        SHARED
        ${JoynrClusterControllerRuntime_HEADERS}
        ${JoynrClusterControllerRuntime_SOURCES}
    )
    target_include_directories(JoynrClusterControllerRuntime SYSTEM PRIVATE ${MOSQUITTO_INCLUDE_DIRS})

    target_include_directories(
        JoynrClusterControllerRuntime
        PRIVATE ${JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES}
    )

    target_include_directories(
        JoynrClusterControllerRuntime
        SYSTEM
        PRIVATE ${Qt5WebSockets_INCLUDE_DIRS}
        PRIVATE ${Qt5Core_INCLUDE_DIRS}
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

    GenerateJoynrExports(
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
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
        # this will flatten directory hierarchy of header files
        # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev
    )
endif(${BUILD_CLUSTER_CONTROLLER})


# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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

add_library(
    JoynrWsRuntime
    SHARED
    ${JoynrRuntime_PRIVATE_HEADERS}
    ${JoynrRuntime_SOURCES}
    ${JoynrWsRuntime_PRIVATE_HEADERS}
    ${JoynrWsRuntime_SOURCES}
)

target_include_directories(
    JoynrWsRuntime
    PRIVATE
    ${JoynrRuntime_INCLUDE_DIRECTORIES}
)

AddClangFormat(JoynrWsRuntime)

AddIncludeWhatYouUse(JoynrWsRuntime)

AddClangTidy(JoynrWsRuntime)

target_link_libraries(
    JoynrWsRuntime
    JoynrCommon
    Joynr
    JoynrJsonSerializer
)

GenerateJoynrExports(
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
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
    # this will flatten directory hierarchy of header files
    # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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

    add_library(
        JoynrDbusRuntime
        SHARED
        ${JoynrRuntime_HEADERS}
        ${JoynrRuntime_SOURCES}
        ${JoynrDbusRuntime_PRIVATE_HEADERS}
        ${JoynrDbusRuntime_SOURCES}
    )

    target_include_directories(
        JoynrDbusRuntime
        PRIVATE ${JoynrRuntime_INCLUDE_DIRECTORIES}
    )

    AddClangFormat(JoynrDbusRuntime)

    AddIncludeWhatYouUse(JoynrDbusRuntime)

    AddClangTidy(JoynrDbusRuntime)

    target_link_libraries(
        JoynrDbusRuntime
        JoynrCommon
        Joynr
    )

    GenerateJoynrExports(
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
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
        # this will flatten directory hierarchy of header files
        # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev
    )
    # so we install header files manually to preserve directory hierarchy
    install(
        DIRECTORY include/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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

    target_include_directories(
        cluster-controller
        PRIVATE ${JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES}
    )

    install(
        TARGETS cluster-controller
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
    )
endif(${BUILD_CLUSTER_CONTROLLER})

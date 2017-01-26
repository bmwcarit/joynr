message(STATUS "#############################################")
message(STATUS "Configuring runtimes...")
message(STATUS "#############################################\n")

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
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
)

include(GenerateJoynrExports)

if(${BUILD_CLUSTER_CONTROLLER})
    option(
        ENABLE_ACCESS_CONTROL
        "Should access control be enabled for the cluster-controller?"
        OFF
    )
    message(STATUS "option ENABLE_ACCESS_CONTROL=" ${ENABLE_ACCESS_CONTROL})

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
        # library type is set via BUILD_SHARED_LIBS
        ${JoynrClusterControllerRuntime_HEADERS}
        ${JoynrClusterControllerRuntime_SOURCES}
    )
    target_include_directories(JoynrClusterControllerRuntime SYSTEM PRIVATE ${MOSQUITTO_INCLUDE_DIRS})

    target_include_directories(
        JoynrClusterControllerRuntime
        PRIVATE ${JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES}
    )

    AddClangFormat(JoynrClusterControllerRuntime)

    AddIncludeWhatYouUse(JoynrClusterControllerRuntime)

    AddClangTidy(JoynrClusterControllerRuntime)

    set_target_properties(
        JoynrClusterControllerRuntime
        PROPERTIES
        SOVERSION "${JOYNR_SOVERSION}"
        VERSION "${JOYNR_VERSION}"
        PUBLIC_INCLUDE_DIRECTORY "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
    )

    GenerateJoynrExports(
        JoynrClusterControllerRuntime
        EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrClusterControllerRuntime_EXPORT_HEADER}"
    )

    target_link_libraries(
        JoynrClusterControllerRuntime
        ${JoynrClusterControllerRuntime_TARGET_LIBRARIES}
    )

    install(
        TARGETS JoynrClusterControllerRuntime
        EXPORT JoynrTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        # this will flatten directory hierarchy of header files
        # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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
    "libjoynr-runtime/LibJoynrRuntime.h"
)

set(
    JoynrRuntime_SOURCES
    "JoynrRuntime.cpp"
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
)

set(
    JoynrWsRuntime_SOURCES
    "libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.cpp"
    "libjoynr-runtime/websocket/JoynrRuntime.cpp"
)

add_library(
    JoynrWsRuntime
    # library type is set via BUILD_SHARED_LIBS
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
)

GenerateJoynrExports(
    JoynrWsRuntime
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrRuntime_EXPORT_HEADER}"
)

set_target_properties(
    JoynrWsRuntime
    PROPERTIES
    SOVERSION "${JOYNR_SOVERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
)

install(
    TARGETS JoynrWsRuntime
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    # this will flatten directory hierarchy of header files
    # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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
    )

    set(
        JoynrDbusRuntime_SOURCES
        "libjoynr-runtime/dbus/LibJoynrDbusRuntime.cpp"
        "libjoynr-runtime/dbus/JoynrRuntime.cpp"
    )

    get_target_property(common_api_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrRuntime_INCLUDE_DIRECTORIES
        ${common_api_PUBLIC_INCLUDE_DIRECTORY}
    )

    add_library(
        JoynrDbusRuntime
        # library type is set via BUILD_SHARED_LIBS
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
        SOVERSION "${JOYNR_SOVERSION}"
        VERSION "${JOYNR_VERSION}"
        PUBLIC_INCLUDE_DIRECTORY "${runtime_PUBLIC_INCLUDE_DIRECTORY}"
    )

    install(
        TARGETS JoynrDbusRuntime
        EXPORT JoynrTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        # this will flatten directory hierarchy of header files
        # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    # so we install header files manually to preserve directory hierarchy
    install(
        DIRECTORY include/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h"
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})


if(${BUILD_CLUSTER_CONTROLLER})
    # executable to start the cluster controller

    # set variables to print CC version
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/git-revision-description")
    include(GetGitRevisionDescription)
    get_git_head_revision(GIT_REFSPEC JOYNR_HEAD_SHA1)
    string(SUBSTRING ${JOYNR_HEAD_SHA1} 0 10 JOYNR_HEAD_SHA1_SHORT)
    set(JOYNR_PACKAGE_REVISION ${JOYNR_VERSION}.${JOYNR_HEAD_SHA1_SHORT})
    string(TIMESTAMP JOYNR_BUILD_TIME)

    # configure JoynrVersion.h
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/JoynrVersion.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/JoynrVersion.h
    )

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

    if(ENABLE_ACCESS_CONTROL)
        target_compile_definitions(
            cluster-controller
            PRIVATE
            JOYNR_ENABLE_ACCESS_CONTROL)
    endif(ENABLE_ACCESS_CONTROL)

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
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif(${BUILD_CLUSTER_CONTROLLER})

message(STATUS "#############################################")
message(STATUS "Configuring runtimes...")
message(STATUS "#############################################\n")

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
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
)

include(GenerateJoynrExports)

get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
set(
    JoynrClusterControllerRuntime_INCLUDE_DIRECTORIES
    ${JoynrRuntime_INCLUDE_DIRECTORIES}
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    ${MOSQUITTO_INCLUDE_DIRS}
)

set(
    JoynrClusterControllerRuntime_TARGET_LIBRARIES
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
    "cluster-controller-runtime/signal-handler/*.h"
)

set(
    JoynrClusterControllerRuntime_HEADERS
    ${JoynrClusterControllerRuntime_PRIVATE_HEADERS}
    ${runtime_PUBLIC_HEADERS}
)

file(
    GLOB JoynrClusterControllerRuntime_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "JoynrRuntime.cpp"
    "cluster-controller-runtime/*.cpp"
    "cluster-controller-runtime/signal-handler/*.cpp"
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

AddClangFormat(JoynrClusterControllerRuntime ${JoynrClusterControllerRuntime_EXPORT_HEADER})

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

AddClangFormat(JoynrWsRuntime ${JoynrRuntime_EXPORT_HEADER})

AddIncludeWhatYouUse(JoynrWsRuntime)

AddClangTidy(JoynrWsRuntime)

target_link_libraries(
    JoynrWsRuntime
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

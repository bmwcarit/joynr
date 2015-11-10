message(STATUS "#############################################")
message(STATUS "Configuring JoynrLib...")
message(STATUS "#############################################\n")

get_target_property(qjson_PUBLIC_INCLUDE_DIRECTORY qjson PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)

set(
    JoynrLib_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


set(
    JoynrLib_INCLUDE_DIRECTORIES
    ${JoynrLib_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${qjson_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrLib_TARGET_LIBRARIES
    JoynrCommon
    qjson
)

set(
    JoynrLib_EXPORT_HEADER
    "include/joynr/JoynrExport.h"
)


file(
    GLOB_RECURSE JoynrLib_PUBLIC_HEADERS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

file(
    GLOB_RECURSE JoynrLib_PRIVATE_HEADERS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "capabilities/*.h"
    "common/*.h"
    "communication-model/*.h"
    "dispatcher/*.h"
    "in-process/*.h"
    "joynr-messaging/*.h"
    "some-ip/*.h"
    "provider/*.h"
    "proxy/*.h"
    "subscription/*.h"
    "websocket/*.h"
    "CapabilitiesRegistrar.h"
    "LibjoynrSettings.h"
)


set(
    JoynrLib_HEADERS
    ${JoynrLib_PRIVATE_HEADERS}
    ${JoynrLib_PUBLIC_HEADERS}
)

file(
    GLOB_RECURSE JoynrLib_SOURCES
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "capabilities/*.cpp"
    "common/*.cpp"
    "communication-model/*.cpp"
    "dispatcher/*.cpp"
    "in-process/*.cpp"
    "joynr-messaging/*.cpp"
    "some-ip/*.cpp"
    "provider/*.cpp"
    "proxy/*.cpp"
    "subscription/*.cpp"
    "websocket/*.cpp"
    "CapabilitiesRegistrar.cpp"
    "LibjoynrSettings.cpp"
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    set(
        JoynrLib_HEADERS
        ${JoynrLib_HEADERS}
        "dbus/DbusMessagingStubFactory.h"
    )

    set(
        JoynrLib_SOURCES
        ${JoynrLib_SOURCES}
        "dbus/DBusMessageRouterAdapter.cpp"
        "dbus/DbusMessagingStubFactory.cpp"
    )

    list(
        APPEND JoynrLib_TARGET_LIBRARIES
        ${DBUS1_LIBRARIES}
        ${COMMONAPI_LIBRARIES}
        ${COMMONAPI_DBUS_LIBRARIES}
    )

    get_target_property(common_api_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrLib_INCLUDE_DIRECTORIES
        ${common_api_PUBLIC_INCLUDE_DIRECTORY}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

include_directories(
    ${JoynrLib_INCLUDE_DIRECTORIES}
)

add_library(
    Joynr
    SHARED
    ${JoynrLib_HEADERS}
    ${JoynrLib_SOURCES}
)

add_clang_format(
    Joynr
    ${JoynrLib_HEADERS}
    ${JoynrLib_SOURCES}
)

set_target_properties(
    Joynr
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrLib_PUBLIC_INCLUDE_DIRECTORY}"
)

generate_export_header(
    Joynr
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrLib_EXPORT_HEADER}"
)

target_link_libraries(Joynr ${JoynrLib_TARGET_LIBRARIES})

install(
    TARGETS Joynr
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

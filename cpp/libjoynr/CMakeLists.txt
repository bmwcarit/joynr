message(STATUS "#############################################")
message(STATUS "Configuring JoynrLib...")
message(STATUS "#############################################\n")

### Add websocketpp ###########################################################

find_package(OpenSSL REQUIRED)
include(CheckOpenSSLImportTargets)

include(AddWebSocketPP)

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)

if(${GENERATE_SOURCES})
    JoynrGenerator(
        OUTPUTPATH "${CMAKE_CURRENT_SOURCE_DIR}"
        MODELPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../basemodel/src/main/franca"
        GENERATION_LANGUAGE "cpp"
    )
endif(${GENERATE_SOURCES})

set(
    JoynrLib_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(
    JoynrLib_INCLUDE_DIRECTORIES
    ${JoynrLib_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrLib_TARGET_LIBRARIES
    JoynrCommon
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
    "exceptions/*.cpp"
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

add_library(
    Joynr
    # library type is set via BUILD_SHARED_LIBS
    ${JoynrLib_HEADERS}
    ${JoynrLib_SOURCES}
)

target_include_directories(
    Joynr
    PRIVATE ${JoynrLib_INCLUDE_DIRECTORIES}
)

target_include_directories(
    Joynr
    SYSTEM
    PUBLIC
        "$<BUILD_INTERFACE:${WEBSOCKETPP_INCLUDE_DIR}>"
        "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>" # necessary, because requirements are not propagated properly in cmake 2.8.x
)

if(NOT USE_PLATFORM_WEBSOCKETPP)
    add_dependencies(Joynr websocketpp)
endif(NOT USE_PLATFORM_WEBSOCKETPP)

AddClangFormat(Joynr ${JoynrLib_EXPORT_HEADER})

AddIncludeWhatYouUse(Joynr)

AddClangTidy(Joynr)

set_target_properties(
    Joynr
    PROPERTIES
    SOVERSION "${JOYNR_SOVERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrLib_PUBLIC_INCLUDE_DIRECTORY}"
)

include(GenerateJoynrExports)
GenerateJoynrExports(
    Joynr
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrLib_EXPORT_HEADER}"
)

target_link_libraries(Joynr
    ${JoynrLib_TARGET_LIBRARIES}
    Boost::thread
    OpenSSL::SSL
)

install(
    TARGETS Joynr
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

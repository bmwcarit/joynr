message(STATUS "#############################################")
message(STATUS "Configuring JoynrCommon...")
message(STATUS "#############################################\n")

configure_file(
    JoynrConfig.h.cmake
    "${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/JoynrConfig.h"
    @ONLY
)

configure_file(
    "RuntimeConfig.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/RuntimeConfig.h"
    @ONLY
)


set(
    JoynrCommon_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(
    JoynrCommon_INCLUDE_DIRECTORIES
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrCommon_TARGET_LIBRARIES
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND JoynrCommon_TARGET_LIBRARIES
        JoynrCommonAPI
    )

    get_target_property(JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrCommon_INCLUDE_DIRECTORIES
        ${JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

set(
    JoynrCommon_EXPORT_HEADER
    "include/joynr/JoynrCommonExport.h"
)

file(
    GLOB_RECURSE JoynrCommon_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

file(
    GLOB JoynrCommon_PRIVATE_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h"
    "in-process/*.h"
    "joynr-messaging/*.h"
    "cache/*.h"
    "capabilities/*.h"
    "rpc/*.h"
    "exceptions/*.h"
    "concurrency/*.h"
)

set(
    JoynrCommon_HEADERS
    ${JoynrCommon_PUBLIC_HEADERS}
    ${JoynrCommon_PRIVATE_HEADERS}
)

file(
    GLOB JoynrCommon_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
    "in-process/*.cpp"
    "joynr-messaging/*.cpp"
    "cache/*.cpp"
    "capabilities/*.cpp"
    "rpc/*.cpp"
    "exceptions/*.cpp"
    "SettingsMerger.cpp"
    "concurrency/*.cpp"
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND JoynrCommon_HEADERS
        "dbus/DbusMessagingStubAdapter.h"
        "dbus/IDbusStubWrapper.h"
        "dbus/DbusMessagingUtil.h"
        "dbus/DbusSettings.h"
    )

    list(
        APPEND JoynrCommon_SOURCES
        "dbus/DbusMessagingSkeleton.cpp"
        "dbus/DbusMessagingStubAdapter.cpp"
        "dbus/DbusMessagingUtil.cpp"
        "dbus/DbusSettings.cpp"
    )

    list(
        APPEND JoynrCommon_TARGET_LIBRARIES
        ${DBUS1_LIBRARIES}
        ${COMMONAPI_LIBRARIES}
        ${COMMONAPI_DBUS_LIBRARIES}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

add_library(
    JoynrCommon
    SHARED
    ${JoynrCommon_HEADERS}
    ${JoynrCommon_SOURCES}
)

set_target_properties(
    JoynrCommon
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}"
    COMPILE_DEFINITIONS CMAKE_JOYNR_SETTINGS_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}/${INSTALL_RES_DIR}"
)

# make spdlog system directory to avoid compiler warnings
target_include_directories(JoynrCommon SYSTEM PUBLIC "$<BUILD_INTERFACE:${spdlog_INCLUDE_DIRS}>")

target_include_directories(
    JoynrCommon
    PRIVATE ${JoynrCommon_INCLUDE_DIRECTORIES}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libjoynr/include
)

generate_export_header(
    JoynrCommon
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrCommon_EXPORT_HEADER}"
)

target_link_libraries(JoynrCommon ${JoynrCommon_TARGET_LIBRARIES})

if(NOT USE_PLATFORM_SPDLOG)
    add_dependencies(JoynrCommon spdlog)
endif(NOT USE_PLATFORM_SPDLOG)

AddClangFormat(JoynrCommon)

AddIncludeWhatYouUse(JoynrCommon)

AddClangTidy(JoynrCommon)

install(
    TARGETS JoynrCommon
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev
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
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-system-services.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-system-services.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-system-services.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-system-services.settings
        @ONLY
    )
else()
    # copy resources for build tree
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-system-services.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-system-services.settings
        @ONLY
    )
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-system-services.settings
        DESTINATION ${INSTALL_RES_DIR}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

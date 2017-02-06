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
)

set(
    JoynrCommon_TARGET_LIBRARIES
    Boost::system
    Boost::thread
)

if(JOYNR_ENABLE_DLT_LOGGING)
    list(
        APPEND JoynrCommon_TARGET_LIBRARIES
        DLT::DLT
    )
endif(JOYNR_ENABLE_DLT_LOGGING)

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
    "include/joynr/serializer/*.h"
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
    # library type is set via BUILD_SHARED_LIBS
    ${JoynrCommon_HEADERS}
    ${JoynrCommon_SOURCES}
)

if(JOYNR_ENABLE_STDOUT_LOGGING)
    target_compile_definitions(
        JoynrCommon
        PUBLIC
        JOYNR_ENABLE_STDOUT_LOGGING)
endif(JOYNR_ENABLE_STDOUT_LOGGING)

if(JOYNR_ENABLE_DLT_LOGGING)
    target_compile_definitions(
        JoynrCommon
        PUBLIC
        JOYNR_ENABLE_DLT_LOGGING)
endif(JOYNR_ENABLE_DLT_LOGGING)

set_target_properties(
    JoynrCommon
    PROPERTIES
    SOVERSION "${JOYNR_SOVERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}"
    COMPILE_DEFINITIONS CMAKE_JOYNR_SETTINGS_INSTALL_DIR="${JOYNR_INSTALL_FULL_SYSCONFDIR}"
)

# make spdlog system directory to avoid compiler warnings
target_include_directories(
    JoynrCommon
    SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${spdlog_INCLUDE_DIRS}>"
)

if(JOYNR_ENABLE_DLT_LOGGING)
target_include_directories(
    JoynrCommon
    SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${dlt_INCLUDE_DIRS}>"
)
endif(JOYNR_ENABLE_DLT_LOGGING)

# add muesli
add_dependencies(JoynrCommon muesli::muesli)
get_target_property(muesli_INCLUDE_DIRECTORIES muesli::muesli INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(JoynrCommon SYSTEM PUBLIC "$<BUILD_INTERFACE:${muesli_INCLUDE_DIRECTORIES}>")

target_include_directories(
    JoynrCommon
    PRIVATE ${JoynrCommon_INCLUDE_DIRECTORIES}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libjoynr/include
)

include(GenerateJoynrExports)
GenerateJoynrExports(
    JoynrCommon
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrCommon_EXPORT_HEADER}"
)

target_link_libraries(JoynrCommon ${JoynrCommon_TARGET_LIBRARIES})

if(NOT USE_PLATFORM_SPDLOG)
    add_dependencies(JoynrCommon spdlog)
endif(NOT USE_PLATFORM_SPDLOG)

if(JOYNR_ENABLE_DLT_LOGGING)
add_dependencies(JoynrCommon DLT::DLT)
endif(JOYNR_ENABLE_DLT_LOGGING)

AddClangFormat(JoynrCommon ${JoynrCommon_EXPORT_HEADER})

AddIncludeWhatYouUse(JoynrCommon)

AddClangTidy(JoynrCommon)

install(
    TARGETS JoynrCommon
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
        DESTINATION ${JOYNR_INSTALL_FULL_SYSCONFDIR}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

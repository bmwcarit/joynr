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
)

get_target_property(qjson_PUBLIC_INCLUDE_DIRECTORY qjson PUBLIC_INCLUDE_DIRECTORY)
get_target_property(log4qt_PUBLIC_INCLUDE_DIRECTORY log4qt PUBLIC_INCLUDE_DIRECTORY)

set(
    JoynrCommon_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(
    JoynrCommon_INCLUDE_DIRECTORIES
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${qjson_PUBLIC_INCLUDE_DIRECTORY}
    ${log4qt_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrCommon_TARGET_LIBRARIES
    qjson
    log4qt
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

include_directories(
    ${JoynrCommon_INCLUDE_DIRECTORIES}
)

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
    "logging/*.h"
    "capabilities/*.h"
    "rpc/*.h"
    "exceptions/*.h"
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
    "logging/joynrlogging_log4qt.cpp"
    "capabilities/*.cpp"
    "rpc/*.cpp"
    "exceptions/*.cpp"
    "SettingsMerger.cpp"
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
        ${JoynrCommon_TARGET_LIBRARIES}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

add_library(
    JoynrCommon
    SHARED
    ${JoynrCommon_HEADERS}
    ${JoynrCommon_SOURCES}
)
qt5_use_modules(JoynrCommon Core WebSockets)

set(COMMENT "running C++ code formatter: clang-format")
set(COMMENT "${COMMENT}\nworking directory: ${CMAKE_CURRENT_SOURCE_DIR}")
set(COMMENT "${COMMENT}\nprocessing header files:\n${JoynrCommon_HEADERS}")
set(COMMENT "${COMMENT}\nprocessing source files:\n${JoynrCommon_SOURCES}")

add_custom_command(
    TARGET JoynrCommon
    PRE_BUILD
    COMMAND clang-format -i ${JoynrCommon_HEADERS} ${JoynrCommon_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "${COMMENT}"
    VERBATIM
)

set_target_properties(
    JoynrCommon
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}"
)

generate_export_header(
    JoynrCommon
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrCommon_EXPORT_HEADER}"
)

target_link_libraries(JoynrCommon ${JoynrCommon_TARGET_LIBRARIES})

install(
    TARGETS JoynrCommon
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
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-system-services.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-system-services.settings)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-system-services.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-system-services.settings)
else()
    # copy resources for build tree
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/default-system-services.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-system-services.settings)
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-system-services.settings
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

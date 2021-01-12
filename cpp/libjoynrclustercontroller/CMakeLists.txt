project(JoynrClusterControllerLib)

add_library(${PROJECT_NAME} SHARED "")
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set(JoynrClusterControllerLib_EXPORT_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/include/joynr/JoynrClusterControllerExport.h"
)
set(EXPORT_HEADER_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")

generate_export_header(
    ${PROJECT_NAME}
    EXPORT_FILE_NAME "${JoynrClusterControllerLib_EXPORT_HEADER}"
    BASE_NAME "JoynrClusterController"
)

add_subdirectory(access-control)
add_subdirectory(capabilities-directory)
add_subdirectory(messaging)
add_subdirectory(mqtt)
add_subdirectory(uds)
add_subdirectory(websocket)

set(PUBLIC_HEADERS
    "${JoynrExportHeader}"

    include/joynr/AbstractGlobalMessagingSkeleton.h
    include/joynr/CapabilitiesStorage.h
    include/joynr/LocalCapabilitiesDirectoryStore.h
    include/joynr/CcMessageRouter.h
    include/joynr/ClusterControllerDirectories.h
    include/joynr/ClusterControllerSettings.h
    include/joynr/ITransportMessageReceiver.h
    include/joynr/ITransportMessageSender.h
    include/joynr/LCDUtil.h
    include/joynr/LocalCapabilitiesDirectory.h
    include/joynr/MqttMessagingSkeleton.h
    include/joynr/MqttReceiver.h
    include/joynr/LcdPendingLookupsHandler.h
)

target_sources(${PROJECT_NAME} PRIVATE
    ${PUBLIC_HEADERS}

    ClusterControllerSettings.cpp
    ClusterControllerCallContext.cpp
)
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC JoynrCCLib::AccessControl
    PUBLIC JoynrCCLib::CapabilitiesDirectory
    PUBLIC JoynrCCLib::CcJoynrMessaging
    PUBLIC JoynrCCLib::CcMessaging
    PUBLIC JoynrCCLib::Mqtt
    PUBLIC JoynrCCLib::CcUds
    PUBLIC JoynrCCLib::CcWebsocket
)


AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "JoynrClusterController"
    SOVERSION "${Joynr_SOVERSION}"
    VERSION "${Joynr_VERSION}"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${JOYNR_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${JOYNR_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${JOYNR_INSTALL_LIBDIR}"
)
install(
    DIRECTORY include/ "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

# Visual Studio compiles to Debug and Release directories
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-uds.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-uds.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-uds.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-uds.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings
        @ONLY
    )
else()
    # copy resources for build tree
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-uds.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-uds.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-clustercontroller.settings
        @ONLY
    )
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-uds.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-clustercontroller.settings
        DESTINATION ${JOYNR_INSTALL_FULL_SYSCONFDIR}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

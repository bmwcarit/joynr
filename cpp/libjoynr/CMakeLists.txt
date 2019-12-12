project(JoynrLib)

add_subdirectory(serializer)
add_subdirectory(logger)
add_subdirectory(util)
add_subdirectory(concurrency)
add_subdirectory(interface)
add_subdirectory(basemodel)
add_subdirectory(capabilities)
add_subdirectory(joynr-messaging)
add_subdirectory(in-process)
add_subdirectory(websocket)

add_library(${PROJECT_NAME} SHARED "")
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Serializer
    PUBLIC Joynr::Logger
    PUBLIC Joynr::Util
    PUBLIC Joynr::Concurrency
    PUBLIC Joynr::Interface
    PUBLIC Joynr::BaseModel
    PUBLIC Joynr::BaseModelGenerated
    PUBLIC Joynr::Capabilities
    PUBLIC Joynr::Messaging
    PUBLIC Joynr::InProcessMessaging
    PUBLIC Joynr::WebSocket
)

target_compile_features(${PROJECT_NAME}
    PUBLIC cxx_std_14
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "Joynr"
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

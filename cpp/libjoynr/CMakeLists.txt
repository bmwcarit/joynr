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

file(GLOB_RECURSE JoynrLib_GENERATED_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "communication-model/generated/*.h"
    "in-process/generated/*.h"
    "joynr-messaging/generated/*.h"
    "proxy/generated/*.h"
    "provider/generated/*.h"
)

file(GLOB_RECURSE JoynrLib_GENERATED_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "communication-model/generated/*.cpp"
    "in-process/generated/*.cpp"
    "joynr-messaging/generated/*.cpp"
    "proxy/generated/*.cpp"
    "provider/generated/*.cpp"
)

set(JoynrLib_PRIVATE_HEADERS
    "in-process/InProcessMessagingStubFactory.h"
    "in-process/InProcessLibJoynrMessagingSkeleton.h"
    "joynr-messaging/dispatcher/ReceivedMessageRunnable.h"
    "joynr-messaging/DummyPlatformSecurityManager.h"
    "websocket/IWebSocketPpClient.h"
    "websocket/WebSocketLibJoynrMessagingSkeleton.h"
    "websocket/WebSocketMessagingStub.h"
    "websocket/WebSocketMessagingStubFactory.h"
    "websocket/WebSocketPpClient.h"
    "websocket/WebSocketPpClientNonTLS.h"
    "websocket/WebSocketPpClientTLS.h"
    "websocket/WebSocketPpReceiver.h"
    "websocket/WebSocketPpSender.h"
)

set(
    JoynrLib_HEADERS
    ${JoynrLib_PRIVATE_HEADERS}
    ${JoynrLib_PUBLIC_HEADERS}
    ${JoynrLib_GENERATED_HEADERS}
)

set(JoynrLib_SOURCES
    "capabilities/LocalDiscoveryAggregator.cpp"
    "capabilities/ParticipantIdStorage.cpp"
    "CapabilitiesRegistrar.cpp"
    "common/MessagingStubFactory.cpp"
    "common/MulticastMessagingSkeletonDirectory.cpp"
    "common/StatusCode.cpp"
    "exceptions/MethodInvocationException.cpp"
    "exceptions/NoCompatibleProviderFoundException.cpp"
    "exceptions/SubscriptionException.cpp"
    "in-process/InProcessAddress.cpp"
    "in-process/InProcessConnectorFactory.cpp"
    "in-process/InProcessDispatcher.cpp"
    "in-process/InProcessLibJoynrMessagingSkeleton.cpp"
    "in-process/InProcessMessagingAddress.cpp"
    "in-process/InProcessMessagingStubFactory.cpp"
    "in-process/InProcessPublicationSender.cpp"
    "joynr-messaging/AbstractJoynrMessagingConnector.cpp"
    "joynr-messaging/BrokerUrl.cpp"
    "joynr-messaging/dispatcher/Dispatcher.cpp"
    "joynr-messaging/dispatcher/ReceivedMessageRunnable.cpp"
    "joynr-messaging/DummyPlatformSecurityManager.cpp"
    "joynr-messaging/HttpMulticastAddressCalculator.cpp"
    "joynr-messaging/JoynrMessageFactory.cpp"
    "joynr-messaging/JoynrMessageSender.cpp"
    "joynr-messaging/JoynrMessagingConnectorFactory.cpp"
    "joynr-messaging/LibJoynrMessageRouter.cpp"
    "joynr-messaging/MessagingSettings.cpp"
    "joynr-messaging/MqttMulticastAddressCalculator.cpp"
    "joynr-messaging/WebSocketMulticastAddressCalculator.cpp"
    "LibjoynrSettings.cpp"
    "provider/AbstractJoynrProvider.cpp"
    "provider/InterfaceRegistrar.cpp"
    "provider/RequestCaller.cpp"
    "proxy/Arbitrator.cpp"
    "proxy/ArbitratorFactory.cpp"
    "proxy/ConnectorFactory.cpp"
    "proxy/DiscoveryQos.cpp"
    "proxy/FixedParticipantArbitrationStrategyFunction.cpp"
    "proxy/KeywordArbitrationStrategyFunction.cpp"
    "proxy/LastSeenArbitrationStrategyFunction.cpp"
    "proxy/ProxyBase.cpp"
    "proxy/ProxyFactory.cpp"
    "proxy/QosArbitrationStrategyFunction.cpp"
    "subscription/BasePublication.cpp"
    "subscription/BroadcastFilterParameters.cpp"
    "subscription/BroadcastSubscriptionRequest.cpp"
    "subscription/BroadcastSubscriptionRequestInformation.cpp"
    "subscription/ISubscriptionManager.cpp"
    "subscription/MulticastPublication.cpp"
    "subscription/MulticastSubscriptionRequest.cpp"
    "subscription/PublicationManager.cpp"
    "subscription/SubscriptionInformation.cpp"
    "subscription/SubscriptionManager.cpp"
    "subscription/SubscriptionPublication.cpp"
    "subscription/SubscriptionReply.cpp"
    "subscription/SubscriptionRequest.cpp"
    "subscription/SubscriptionRequestInformation.cpp"
    "subscription/SubscriptionStop.cpp"
    "websocket/WebSocketLibJoynrMessagingSkeleton.cpp"
    "websocket/WebSocketMessagingStub.cpp"
    "websocket/WebSocketMessagingStubFactory.cpp"
    "websocket/WebSocketPpClientTLS.cpp"
    "websocket/WebSocketSettings.cpp"
    ${JoynrLib_GENERATED_SOURCES}
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

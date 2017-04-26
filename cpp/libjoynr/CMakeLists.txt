message(STATUS "#############################################")
message(STATUS "Configuring JoynrLib...")
message(STATUS "#############################################\n")

### Add websocketpp ###########################################################

find_package(OpenSSL REQUIRED)
include(CheckOpenSSLImportTargets)

include(AddWebSocketPP)

# generate JoynrVersion.h which contains the version of joynr
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/git-revision-description")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC JOYNR_HEAD_SHA1)
string(SUBSTRING ${JOYNR_HEAD_SHA1} 0 10 JOYNR_HEAD_SHA1_SHORT)
set(JOYNR_PACKAGE_REVISION ${JOYNR_VERSION}.${JOYNR_HEAD_SHA1_SHORT})
string(TIMESTAMP JOYNR_BUILD_TIME)

configure_file(
    "common/JoynrVersion.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/JoynrVersion.h"
    @ONLY
)

configure_file(
    "common/RuntimeConfig.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/RuntimeConfig.h"
    @ONLY
)

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
)

set(
    JoynrLib_TARGET_LIBRARIES
    Boost::system
    Boost::thread
    smrf::smrf
)

if(JOYNR_ENABLE_DLT_LOGGING)
    list(
        APPEND JoynrLib_TARGET_LIBRARIES
        DLT::DLT
    )
endif(JOYNR_ENABLE_DLT_LOGGING)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND JoynrLib_TARGET_LIBRARIES
        JoynrCommonAPI
    )

    get_target_property(JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND Joynr_INCLUDE_DIRECTORIES
        ${JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

set(
    JoynrLib_EXPORT_HEADER
    "include/joynr/JoynrExport.h"
)

file(
    GLOB_RECURSE JoynrLib_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
    "include/joynr/serializer/*.h"
    "include/joynr/exceptions/*.h"
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
    "common/CallContextStorage.h"
    "in-process/InProcessMessagingSkeleton.h"
    "in-process/InProcessMessagingStubFactory.h"
    "in-process/InProcessMessagingStub.h"
    "joynr-messaging/dispatcher/ReceivedMessageRunnable.h"
    "joynr-messaging/DummyPlatformSecurityManager.h"
    "websocket/IWebSocketPpClient.h"
    "websocket/WebSocketLibJoynrMessagingSkeleton.h"
    "websocket/WebSocketMessagingStubFactory.h"
    "websocket/WebSocketMessagingStub.h"
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
    "common/CallContext.cpp"
    "common/CallContextStorage.cpp"
    "common/CapabilityUtils.cpp"
    "common/concurrency/BlockingQueue.cpp"
    "common/concurrency/DelayedScheduler.cpp"
    "common/concurrency/Runnable.cpp"
    "common/concurrency/Semaphore.cpp"
    "common/concurrency/SingleThreadedDelayedScheduler.cpp"
    "common/concurrency/Thread.cpp"
    "common/concurrency/ThreadPool.cpp"
    "common/concurrency/ThreadPoolDelayedScheduler.cpp"
    "common/DispatcherUtils.cpp"
    "common/InterfaceAddress.cpp"
    "common/MessagingQos.cpp"
    "common/MessagingStubFactory.cpp"
    "common/MulticastMessagingSkeletonDirectory.cpp"
    "common/MulticastReceiverDirectory.cpp"
    "common/MulticastSubscriptionQos.cpp"
    "common/ObjectWithDecayTime.cpp"
    "common/OnChangeSubscriptionQos.cpp"
    "common/OnChangeWithKeepAliveSubscriptionQos.cpp"
    "common/PeriodicSubscriptionQos.cpp"
    "common/rpc/BaseReply.cpp"
    "common/rpc/OneWayRequest.cpp"
    "common/rpc/Reply.cpp"
    "common/rpc/Request.cpp"
    "common/Settings.cpp"
    "common/StatusCode.cpp"
    "common/SteadyTimer.cpp"
    "common/SubscriptionQos.cpp"
    "common/SubscriptionUtil.cpp"
    "common/SystemServicesSettings.cpp"
    "common/UnicastSubscriptionQos.cpp"
    "common/Url.cpp"
    "common/Util.cpp"
    "exceptions/JoynrException.cpp"
    "exceptions/MethodInvocationException.cpp"
    "exceptions/NoCompatibleProviderFoundException.cpp"
    "exceptions/SubscriptionException.cpp"
    "in-process/InProcessAddress.cpp"
    "in-process/InProcessConnectorFactory.cpp"
    "in-process/InProcessDispatcher.cpp"
    "in-process/InProcessMessagingAddress.cpp"
    "in-process/InProcessMessagingSkeleton.cpp"    
    "in-process/InProcessMessagingStub.cpp"
    "in-process/InProcessMessagingStubFactory.cpp"
    "in-process/InProcessPublicationSender.cpp"
    "joynr-messaging/AbstractJoynrMessagingConnector.cpp"
    "joynr-messaging/AbstractMessageRouter.cpp"
    "joynr-messaging/BrokerUrl.cpp"
    "joynr-messaging/dispatcher/Dispatcher.cpp"
    "joynr-messaging/dispatcher/ReceivedMessageRunnable.cpp"
    "joynr-messaging/DummyPlatformSecurityManager.cpp"
    "joynr-messaging/HttpMulticastAddressCalculator.cpp"
    "joynr-messaging/ImmutableMessage.cpp"
    "joynr-messaging/JoynrMessage.cpp"
    "joynr-messaging/JoynrMessagingConnectorFactory.cpp"
    "joynr-messaging/LibJoynrMessageRouter.cpp"
    "joynr-messaging/MessageQueue.cpp"
    "joynr-messaging/MessageSender.cpp"    
    "joynr-messaging/MessagingSettings.cpp"
    "joynr-messaging/MqttMulticastAddressCalculator.cpp"
    "joynr-messaging/MulticastMatcher.cpp"
    "joynr-messaging/MutableMessage.cpp"
    "joynr-messaging/MutableMessageFactory.cpp"    
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
        "dbus/DbusMessagingStubAdapter.h"
        "dbus/DbusMessagingStubFactory.h"
        "dbus/IDbusStubWrapper.h"
        "dbus/DbusMessagingUtil.h"
        "dbus/DbusSettings.h"
    )

    set(
        JoynrLib_SOURCES
        ${JoynrLib_SOURCES}
        "dbus/DBusMessageRouterAdapter.cpp"
        "dbus/DbusMessagingSkeleton.cpp"
        "dbus/DbusMessagingStubAdapter.cpp"
        "dbus/DbusMessagingStubFactory.cpp"
        "dbus/DbusMessagingUtil.cpp"
        "dbus/DbusSettings.cpp"
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
    SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${spdlog_INCLUDE_DIRS}>"
        "$<BUILD_INTERFACE:${WEBSOCKETPP_INCLUDE_DIR}>"
        "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>" # necessary, because requirements are not propagated properly in cmake 2.8.x
)

if(JOYNR_ENABLE_STDOUT_LOGGING)
    target_compile_definitions(
        Joynr
        PUBLIC
        JOYNR_ENABLE_STDOUT_LOGGING)
endif(JOYNR_ENABLE_STDOUT_LOGGING)

if(JOYNR_ENABLE_DLT_LOGGING)
    target_compile_definitions(
        Joynr
        PUBLIC
        JOYNR_ENABLE_DLT_LOGGING)
    
    target_include_directories(
        Joynr
        SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${dlt_INCLUDE_DIRS}>"
    )
endif(JOYNR_ENABLE_DLT_LOGGING)

add_dependencies(Joynr muesli::muesli)
get_target_property(muesli_INCLUDE_DIRECTORIES muesli::muesli INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(Joynr SYSTEM PUBLIC "$<BUILD_INTERFACE:${muesli_INCLUDE_DIRECTORIES}>")

if(NOT USE_PLATFORM_SPDLOG)
    add_dependencies(Joynr spdlog)
endif(NOT USE_PLATFORM_SPDLOG)

if(JOYNR_ENABLE_DLT_LOGGING)
add_dependencies(Joynr DLT::DLT)
endif(JOYNR_ENABLE_DLT_LOGGING)

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
    COMPILE_DEFINITIONS CMAKE_JOYNR_SETTINGS_INSTALL_DIR="${JOYNR_INSTALL_FULL_SYSCONFDIR}"
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


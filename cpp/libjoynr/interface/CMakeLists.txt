project(Interface)

set(SOURCES
    exceptions/JoynrException.cpp

    joynr-messaging/JoynrMessagingConnectorFactory.cpp
    joynr-messaging/MulticastMatcher.cpp

    rpc/BaseReply.cpp

    subscription/BasePublication.cpp
    subscription/ISubscriptionManager.cpp

    CallContext.cpp
    MessagingQos.cpp
    SubscriptionQos.cpp
)

set(PUBLIC_HEADERS
    include/joynr/exceptions/JoynrException.h

    include/joynr/AbstractBroadcastListener.h
    include/joynr/BasePublication.h
    include/joynr/BaseReply.h
    include/joynr/CallContext.h
    include/joynr/CallContextStorage.h
    include/joynr/ContextStorage.h
    include/joynr/IDispatcher.h
    include/joynr/IJoynrProvider.h
    include/joynr/IKeychain.h
    include/joynr/IMessageRouter.h
    include/joynr/IMessageSender.h
    include/joynr/IMessagingMulticastSubscriber.h
    include/joynr/IMessagingStub.h
    include/joynr/IMessagingStubFactory.h
    include/joynr/IMiddlewareMessagingStubFactory.h
    include/joynr/IMulticastAddressCalculator.h
    include/joynr/IPlatformSecurityManager.h
    include/joynr/IProxyBuilder.h
    include/joynr/IProxyBuilderBase.h
    include/joynr/IPublicationSender.h
    include/joynr/IReplyCaller.h
    include/joynr/IRequestCallerDirectory.h
    include/joynr/IRequestInterpreter.h
    include/joynr/ISubscriptionCallback.h
    include/joynr/ISubscriptionListener.h
    include/joynr/ISubscriptionManager.h
    include/joynr/ITimeoutListener.h
    include/joynr/ITransportStatus.h
    include/joynr/IWebSocketSendInterface.h
    include/joynr/JoynrMessagingConnectorFactory.h
    include/joynr/MessagingQos.h
    include/joynr/MessagingQosEffort.h
    include/joynr/MulticastMatcher.h
    include/joynr/SubscriptionQos.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Util
    PUBLIC Joynr::Serializer
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

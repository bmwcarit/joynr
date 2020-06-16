project(Messaging)

set(SOURCES
    dispatcher/Dispatcher.cpp
    dispatcher/ReceivedMessageRunnable.cpp

    AbstractMessageRouter.cpp
    BrokerUrl.cpp
    DummyPlatformSecurityManager.cpp
    HttpMulticastAddressCalculator.cpp
    ImmutableMessage.cpp
    InterfaceAddress.cpp
    LibJoynrMessageRouter.cpp
    MessageSender.cpp
    MessagingSettings.cpp
    MessagingStubFactory.cpp
    MqttMulticastAddressCalculator.cpp
    MulticastMessagingSkeletonDirectory.cpp
    MutableMessage.cpp
    MutableMessageFactory.cpp
    RoutingTable.cpp
    UdsMulticastAddressCalculator.cpp
    WebSocketMulticastAddressCalculator.cpp
)

set(PRIVATE_HEADERS
    dispatcher/ReceivedMessageRunnable.h

    DummyPlatformSecurityManager.h
)

set(PUBLIC_HEADERS
    include/joynr/AbstractMessageRouter.h
    include/joynr/BrokerUrl.h
    include/joynr/Directory.h
    include/joynr/Dispatcher.h
    include/joynr/HttpMulticastAddressCalculator.h
    include/joynr/ImmutableMessage.h
    include/joynr/InProcessMessagingAddress.h
    include/joynr/InterfaceAddress.h
    include/joynr/LibJoynrDirectories.h
    include/joynr/LibJoynrMessageRouter.h
    include/joynr/Message.h
    include/joynr/MessageQueue.h
    include/joynr/MessageSender.h
    include/joynr/MessagingSettings.h
    include/joynr/MessagingStubFactory.h
    include/joynr/MqttMulticastAddressCalculator.h
    include/joynr/MulticastMessagingSkeletonDirectory.h
    include/joynr/MutableMessage.h
    include/joynr/MutableMessageFactory.h
    include/joynr/ProxyBuilder.h
    include/joynr/RoutingTable.h
    include/joynr/UdsMulticastAddressCalculator.h
    include/joynr/WebSocketMulticastAddressCalculator.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Boost::system
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::BaseModel
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

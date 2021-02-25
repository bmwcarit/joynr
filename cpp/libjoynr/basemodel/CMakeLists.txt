project(BaseModel)

set(GeneratedDir "${CMAKE_CURRENT_SOURCE_DIR}/generated")

if(${GENERATE_SOURCES})
    JoynrGenerator(
        OUTPUTPATH "${GeneratedDir}"
        MODELPATH "${CMAKE_SOURCE_DIR}/../basemodel/src/main/franca"
        GENERATION_LANGUAGE "cpp"
    )
endif()

add_subdirectory("${GeneratedDir}")

set(PUBLIC_HEADERS
    include/joynr/access-control/IAccessController.h

    include/joynr/exceptions/JoynrExceptionUtil.h
    include/joynr/exceptions/MethodInvocationException.h
    include/joynr/exceptions/NoCompatibleProviderFoundException.h
    include/joynr/exceptions/SubscriptionException.h

    include/joynr/AbstractJoynrMessagingConnector.h
    include/joynr/AbstractJoynrProvider.h
    include/joynr/ArbitrationResult.h
    include/joynr/ArbitrationStrategyFunction.h
    include/joynr/Arbitrator.h
    include/joynr/ArbitratorFactory.h
    include/joynr/BroadcastFilterParameters.h
    include/joynr/BroadcastSubscriptionRequest.h
    include/joynr/BroadcastSubscriptionRequestInformation.h
    include/joynr/DiscoveryQos.h
    include/joynr/FixedParticipantArbitrationStrategyFunction.h
    include/joynr/InterfaceRegistrar.h
    include/joynr/KeywordArbitrationStrategyFunction.h
    include/joynr/LastSeenArbitrationStrategyFunction.h
    include/joynr/LibjoynrSettings.h
    include/joynr/MulticastBroadcastListener.h
    include/joynr/MulticastPublication.h
    include/joynr/MulticastReceiverDirectory.h
    include/joynr/MulticastSubscriptionCallback.h
    include/joynr/MulticastSubscriptionQos.h
    include/joynr/MulticastSubscriptionRequest.h
    include/joynr/OnChangeSubscriptionQos.h
    include/joynr/OnChangeWithKeepAliveSubscriptionQos.h
    include/joynr/OneWayRequest.h
    include/joynr/PeriodicSubscriptionQos.h
    include/joynr/ProxyBase.h
    include/joynr/ProxyFactory.h
    include/joynr/PublicationManager.h
    include/joynr/QosArbitrationStrategyFunction.h
    include/joynr/ReplyCaller.h
    include/joynr/ReplyInterpreter.h
    include/joynr/Request.h
    include/joynr/RequestCaller.h
    include/joynr/RequestCallerFactory.h
    include/joynr/Reply.h
    include/joynr/SubscriptionAttributeListener.h
    include/joynr/SubscriptionCallback.h
    include/joynr/SubscriptionInformation.h
    include/joynr/SubscriptionListener.h
    include/joynr/SubscriptionManager.h
    include/joynr/SubscriptionPublication.h
    include/joynr/SubscriptionReply.h
    include/joynr/SubscriptionRequest.h
    include/joynr/SubscriptionRequestInformation.h
    include/joynr/SubscriptionStop.h
    include/joynr/SubscriptionUtil.h
    include/joynr/UnicastBroadcastListener.h
    include/joynr/UnicastSubscriptionCallback.h
    include/joynr/UnicastSubscriptionQos.h

    include/joynr/infrastructure/DacTypes/DomainRoleEntry.h
    include/joynr/infrastructure/DacTypes/Role.h
)

set(SOURCES
    communication-model/datatypes/infrastructure/DacTypes/DomainRoleEntry.cpp
    communication-model/datatypes/infrastructure/DacTypes/Role.cpp

    exceptions/JoynrExceptionUtil.cpp
    exceptions/MethodInvocationException.cpp
    exceptions/NoCompatibleProviderFoundException.cpp
    exceptions/SubscriptionException.cpp

    joynr-messaging/AbstractJoynrMessagingConnector.cpp

    provider/AbstractJoynrProvider.cpp
    provider/InterfaceRegistrar.cpp
    provider/RequestCaller.cpp

    proxy/ArbitrationResult.cpp
    proxy/Arbitrator.cpp
    proxy/ArbitratorFactory.cpp
    proxy/DiscoveryQos.cpp
    proxy/FixedParticipantArbitrationStrategyFunction.cpp
    proxy/KeywordArbitrationStrategyFunction.cpp
    proxy/LastSeenArbitrationStrategyFunction.cpp
    proxy/ProxyBase.cpp
    proxy/ProxyFactory.cpp
    proxy/QosArbitrationStrategyFunction.cpp

    rpc/OneWayRequest.cpp
    rpc/Reply.cpp
    rpc/Request.cpp

    subscription/BroadcastFilterParameters.cpp
    subscription/BroadcastSubscriptionRequest.cpp
    subscription/BroadcastSubscriptionRequestInformation.cpp
    subscription/MulticastPublication.cpp
    subscription/MulticastSubscriptionRequest.cpp
    subscription/PublicationManager.cpp
    subscription/SubscriptionInformation.cpp
    subscription/SubscriptionManager.cpp
    subscription/SubscriptionPublication.cpp
    subscription/SubscriptionReply.cpp
    subscription/SubscriptionRequest.cpp
    subscription/SubscriptionRequestInformation.cpp
    subscription/SubscriptionStop.cpp

    LibjoynrSettings.cpp
    MulticastReceiverDirectory.cpp
    MulticastSubscriptionQos.cpp
    OnChangeSubscriptionQos.cpp
    OnChangeWithKeepAliveSubscriptionQos.cpp
    PeriodicSubscriptionQos.cpp
    SubscriptionUtil.cpp
    UnicastSubscriptionQos.cpp
)

add_library(${PROJECT_NAME} OBJECT
    ${PUBLIC_HEADERS} ${SOURCES}
)
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::BaseModelGenerated
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

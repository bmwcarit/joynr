///*
// * #%L
// * %%
// * Copyright (C) 2011 - 2017 BMW Car IT GmbH
// * %%
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// * 
// *      http://www.apache.org/licenses/LICENSE-2.0
// * 
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// * #L%
// */

//#include <cstdint>
//#include <chrono>
//#include <memory>

//#include <gtest/gtest.h>
//#include <gmock/gmock.h>

//#include "joynr/CcMessageRouter.h"
//#include "joynr/LibJoynrMessageRouter.h"

//#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
//#include "joynr/InProcessMessagingAddress.h"
//#include "joynr/MessagingStubFactory.h"
//#include "joynr/MqttMulticastAddressCalculator.h"
//#include "joynr/MessageQueue.h"
//#include "joynr/MulticastMessagingSkeletonDirectory.h"
//#include "joynr/Semaphore.h"
//#include "joynr/SingleThreadedIOService.h"
//#include "joynr/system/RoutingTypes/ChannelAddress.h"
//#include "joynr/system/RoutingTypes/MqttAddress.h"
//#include "joynr/system/RoutingTypes/WebSocketAddress.h"
//#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
//#include "joynr/WebSocketMulticastAddressCalculator.h"

//#include "tests/utils/MockObjects.h"

//using ::testing::InvokeArgument;
//using ::testing::Pointee;
//using ::testing::Return;
//using ::testing::Truly;

//using namespace joynr;

//ACTION_P(ReleaseSemaphore, semaphore)
//{
//    semaphore->notify();
//}

//class CombinedMessageRoutersTest : public ::testing::Test {
//public:
//    CombinedMessageRoutersTest() :
//        singleThreadedIOService(),
//        settings(),
//        messagingSettings(settings),
//        messageQueue(nullptr),
//        messagingStubFactory(nullptr),
//        messageRouter(nullptr),
//        joynrMessage(),
//        multicastMessagingSkeletonDirectory(std::make_shared<MulticastMessagingSkeletonDirectory>()),
//        brokerURL("mqtt://globalTransport.example.com"),
//        mqttTopic(""),
//        globalTransport(std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerURL, mqttTopic))
//    {
//        singleThreadedIOService.start();
//        auto messageQueue = std::make_unique<MessageQueue>();
//        this->messageQueue = messageQueue.get();

//        messagingStubFactory = std::make_shared<MockMessagingStubFactory>();

//        std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
//                std::make_unique<MqttMulticastAddressCalculator>(globalTransport);

//        messageRouter = std::make_unique<LibJoynrMessageRouter>(
//            messagingStubFactory,
//            multicastMessagingSkeletonDirectory,
//            std::unique_ptr<IPlatformSecurityManager>(),
//            singleThreadedIOService.getIOService(),
//            std::move(addressCalculator),
//            6,
//            std::move(messageQueue)
//        );

//        // provision global capabilities directory
//        auto addressCapabilitiesDirectory =
//                std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
//                    messagingSettings.getCapabilitiesDirectoryUrl() + messagingSettings.getCapabilitiesDirectoryChannelId() + "/",
//                    messagingSettings.getCapabilitiesDirectoryChannelId());
//        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);

//        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
//        joynrMessage.setHeaderExpiryDate(now + std::chrono::milliseconds(100));
//        joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
//    }

//    ~CombinedMessageRoutersTest() override {
//        std::remove(settingsFileName.c_str());
//    }

//protected:
//    void routeMessageToAddress();

//    SingleThreadedIOService singleThreadedIOService;
//    std::string settingsFileName;
//    Settings settings;
//    MessagingSettings messagingSettings;
//    MessageQueue* messageQueue;
//    std::shared_ptr<MockMessagingStubFactory> messagingStubFactory;
//    std::unique_ptr<LibJoynrMessageRouter> messageRouter;
//    JoynrMessage joynrMessage;
//    std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory;
//    std::string brokerURL;
//    std::string mqttTopic;
//    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> globalTransport;
//    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> localTransport;
//};

//TEST_F(CombinedMessageRoutersTest, routeMulticastMessageFromLocalProvider_multicastMsgIsSentToAllMulticastReceivers) {

//    MockRoutingProxy *mockRoutingProxy = new MockRoutingProxy();

//    ON_CALL(
//        *mockRoutingProxy,
//        addMulticastReceiverAsync(_,_,_,_,_)
//    ).WillByDefault(
//        DoAll(
//            InvokeArgument<3>(),
//            Return(nullptr)
//        )
//    );

//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
//                joynr::system::RoutingTypes::WebSocketProtocol::Enum::WS,
//                "host",
//                4242,
//                "path"
//    );

//    std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
//            std::make_unique<WebSocketMulticastAddressCalculator>(parentAddress);

//    messageRouter = std::make_unique<MessageRouter>(
//                messagingStubFactory,
//                std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                singleThreadedIOService.getIOService(),
//                std::move(addressCalculator)
//    );

//    messageRouter->setParentRouter(
//                std::unique_ptr<MockRoutingProxy>(mockRoutingProxy),
//                parentAddress,
//                std::string("parentParticipantId")
//    );

//    const std::string subscriberParticipantId1("subscriberPartId");
//    const std::string providerParticipantId("providerParticipantId");
//    const std::string multicastNameAndPartitions("multicastName/partition0");
//    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
//    const std::shared_ptr<const joynr::InProcessMessagingAddress> expectedAddress =
//            std::make_shared<const joynr::InProcessMessagingAddress>();

//    messageRouter->addProvisionedNextHop(providerParticipantId, parentAddress);
//    messageRouter->addProvisionedNextHop(subscriberParticipantId1, expectedAddress);

//    messageRouter->addMulticastReceiver(
//        multicastId,
//        subscriberParticipantId1,
//        providerParticipantId,
//        [](){ },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
//    );

//    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*parentAddress)))).Times(1);
//    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress)))).Times(1);

//    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
//    joynrMessage.setHeaderFrom(providerParticipantId);
//    joynrMessage.setHeaderTo(multicastId);

//    messageRouter->route(joynrMessage);
//}

//TEST_F(CombinedMessageRoutersTest, addMulticastReceiver_ChildRouter_callsParentRouterIfProviderAddressNotAvailable) {
//    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

//    const std::string multicastId("multicastId");
//    const std::string subscriberParticipantId("subscriberParticipantId");
//    const std::string providerParticipantId("providerParticipantId");

//    EXPECT_CALL(*mockRoutingProxy, resolveNextHopAsync(providerParticipantId,_,_))
//            .WillOnce(
//                DoAll(
//                    InvokeArgument<2>(joynr::exceptions::JoynrRuntimeException("testException")),
//                    Return(nullptr)))
//            .WillOnce(DoAll(InvokeArgument<1>(false), Return(nullptr)))
//            .WillOnce(DoAll(InvokeArgument<1>(true), Return(nullptr)));

//    EXPECT_CALL(*mockRoutingProxy, addMulticastReceiverAsync(
//                    multicastId,
//                    subscriberParticipantId,
//                    providerParticipantId,
//                    _,
//                    _))
//            .WillOnce(
//                DoAll(
//                    InvokeArgument<3>(),
//                    Return(nullptr)
//                )
//            );;

//    messageRouter = std::make_unique<MessageRouter>(
//                std::move(messagingStubFactory),
//                std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                singleThreadedIOService.getIOService(),
//                nullptr);
//    // Set all attributes which are required to make the message router a child message router.
//    messageRouter->setParentRouter(
//                std::move(mockRoutingProxy),
//                parentAddress,
//                std::string("parentParticipantId"));

//    Semaphore errorCallbackCalled;
//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        []() { FAIL() << "onSuccess called"; },
//        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
//        {
//            errorCallbackCalled.notify();
//        });
//    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        []() { FAIL() << "onSuccess called"; },
//        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
//        {
//            errorCallbackCalled.notify();
//        });
//    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

//    bool hasNextHop = false;
//    messageRouter->resolveNextHop(
//                providerParticipantId,
//                [&hasNextHop](const bool& resolved) { hasNextHop = resolved; },
//                nullptr);
//    EXPECT_FALSE(hasNextHop);

//    Semaphore successCallbackCalled;
//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        [&successCallbackCalled]() { successCallbackCalled.notify(); },
//        [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
//    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

//    messageRouter->resolveNextHop(
//                providerParticipantId,
//                [&successCallbackCalled](const bool& resolved) {
//                    if (resolved) {
//                        successCallbackCalled.notify();
//                    } else {
//                        FAIL() << "resolveNextHop failed";
//                    }
//                },
//                nullptr); // not called in resolveNextHop
//    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
//}

//TEST_F(CombinedMessageRoutersTest, removeMulticastReceiver_ChildRouter_CallsParentRouter) {
//    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
//    auto mockRoutingProxyRef = mockRoutingProxy.get();
//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

//    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
//                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                                                    singleThreadedIOService.getIOService(),
//                                                    nullptr);

//    // Set all attributes which are required to make the message router a child message router.
//    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

//    const std::string multicastId("multicastId");
//    const std::string subscriberParticipantId("subscriberParticipantId");
//    const std::string providerParticipantId("providerParticipantId");

//    messageRouter->addProvisionedNextHop(providerParticipantId, parentAddress);

//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        []() { },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

//    // Call shall be forwarded to the parent proxy
//    EXPECT_CALL(*mockRoutingProxyRef,
//        removeMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _));

//    messageRouter->removeMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        []() { },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
//}

//TEST_F(CombinedMessageRoutersTest, removeMulticastReceiverOfInProcessProvider_ChildRouter_callsParentRouter) {
//    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
//    auto mockRoutingProxyRef = mockRoutingProxy.get();
//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

//    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
//                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                                                    singleThreadedIOService.getIOService(),
//                                                    nullptr);

//    // Set all attributes which are required to make the message router a child message router.
//    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

//    const std::string multicastId("multicastId");
//    const std::string subscriberParticipantId("subscriberParticipantId");
//    const std::string providerParticipantId("providerParticipantId");

//    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();
//    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
//    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        []() { },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

//    EXPECT_CALL(*mockRoutingProxyRef,
//        removeMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
//            .Times(1)
//            .WillOnce(
//                DoAll(
//                    InvokeArgument<3>(),
//                    Return(nullptr)
//                )
//            );

//    Semaphore successCallbackCalled;
//    messageRouter->removeMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        [&successCallbackCalled]() { successCallbackCalled.notify(); },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
//    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
//}

//TEST_F(CombinedMessageRoutersTest, addMulticastReceiver_ChildRouter_callsParentRouter) {
//    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
//    auto mockRoutingProxyRef = mockRoutingProxy.get();
//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

//    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
//                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                                                    singleThreadedIOService.getIOService(),
//                                                    nullptr);
//    // Set all attributes which are required to make the message router a child message router.
//    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

//    const std::string multicastId("multicastId");
//    const std::string subscriberParticipantId("subscriberParticipantId");
//    const std::string providerParticipantId("providerParticipantId");
//    messageRouter->addProvisionedNextHop(providerParticipantId, parentAddress);

//    // Call shall be forwarded to the parent proxy
//    EXPECT_CALL(*mockRoutingProxyRef,
//        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _));

//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        []() { },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
//}

//TEST_F(CombinedMessageRoutersTest, addMulticastReceiverForWebSocketProvider_ChildRouter_callsParentRouter) {
//    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
//    auto mockRoutingProxyRef = mockRoutingProxy.get();
//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

//    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
//                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                                                    singleThreadedIOService.getIOService(),
//                                                    nullptr);
//    // Set all attributes which are required to make the message router a child message router.
//    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

//    const std::string multicastId("multicastId");
//    const std::string subscriberParticipantId("subscriberParticipantId");

//    const std::string providerParticipantId("providerParticipantId");
//    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
//    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

//    EXPECT_CALL(*mockRoutingProxyRef,
//        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
//            .Times(1)
//            .WillOnce(
//                DoAll(
//                    InvokeArgument<3>(),
//                    Return(nullptr)
//                )
//            );

//    Semaphore successCallbackCalled;
//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        [&successCallbackCalled]() { successCallbackCalled.notify(); },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
//    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
//}

//TEST_F(CombinedMessageRoutersTest, addMulticastReceiverForInProcessProvider_ChildRouter_callsParentRouter) {
//    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
//    auto mockRoutingProxyRef = mockRoutingProxy.get();
//    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

//    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
//                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
//                                                    singleThreadedIOService.getIOService(),
//                                                    nullptr);
//    // Set all attributes which are required to make the message router a child message router.
//    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

//    const std::string multicastId("multicastId");
//    const std::string subscriberParticipantId("subscriberParticipantId");

//    const std::string providerParticipantId("providerParticipantId");
//    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();
//    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
//    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

//    EXPECT_CALL(*mockRoutingProxyRef,
//        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
//            .Times(1)
//            .WillOnce(
//                DoAll(
//                    InvokeArgument<3>(),
//                    Return(nullptr)
//                )
//            );

//    Semaphore successCallbackCalled;
//    messageRouter->addMulticastReceiver(multicastId,
//        subscriberParticipantId,
//        providerParticipantId,
//        [&successCallbackCalled]() { successCallbackCalled.notify(); },
//        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
//    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
//}

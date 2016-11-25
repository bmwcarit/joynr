/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#include <cstdint>
#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Semaphore.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/MessageRouter.h"
#include "tests/utils/MockObjects.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MessageQueue.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"

using ::testing::InvokeArgument;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::Truly;

using namespace joynr;

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}

class MessageRouterTest : public ::testing::Test {
public:
    MessageRouterTest() :
        singleThreadedIOService(),
        settings(),
        messagingSettings(settings),
        messageQueue(nullptr),
        messagingStubFactory(nullptr),
        messageRouter(nullptr),
        joynrMessage(),
        multicastMessagingSkeletonDirectory(std::make_shared<MulticastMessagingSkeletonDirectory>()),
        brokerURL("mqtt://globalTransport.example.com"),
        mqttTopic(""),
        globalTransport(std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerURL, mqttTopic))
    {
        singleThreadedIOService.start();

        createNewMessageRouter();

        // provision global capabilities directory
        auto addressCapabilitiesDirectory =
                std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                    messagingSettings.getCapabilitiesDirectoryUrl() + messagingSettings.getCapabilitiesDirectoryChannelId() + "/",
                    messagingSettings.getCapabilitiesDirectoryChannelId());
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);
        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        joynrMessage.setHeaderExpiryDate(now + std::chrono::milliseconds(100));
        joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    }

    ~MessageRouterTest() {
        std::remove(settingsFileName.c_str());
    }

protected:

    void createNewMessageRouter()
    {
        auto messageQueue = std::make_unique<MessageQueue>();
        this->messageQueue = messageQueue.get();

        messagingStubFactory = std::make_shared<MockMessagingStubFactory>();

        std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
                std::make_unique<MqttMulticastAddressCalculator>(globalTransport);

        messageRouter = std::make_unique<MessageRouter>(
            messagingStubFactory,
            multicastMessagingSkeletonDirectory,
            std::unique_ptr<IPlatformSecurityManager>(),
            singleThreadedIOService.getIOService(),
            std::move(addressCalculator),
            6,
            std::move(messageQueue)
        );
    }

    SingleThreadedIOService singleThreadedIOService;
    std::string settingsFileName;
    Settings settings;
    MessagingSettings messagingSettings;
    MessageQueue* messageQueue;
    std::shared_ptr<MockMessagingStubFactory> messagingStubFactory;
    std::unique_ptr<MessageRouter> messageRouter;
    JoynrMessage joynrMessage;
    std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory;
    std::string brokerURL;
    std::string mqttTopic;
    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> globalTransport;
    void routeMessageToAddress();

private:
    DISALLOW_COPY_AND_ASSIGN(MessageRouterTest);
};

TEST_F(MessageRouterTest, DISABLED_routeDelegatesToStubFactory){
    // cH: this thest doesn't make sense anymore, since the MessageRouter
    // will create the MessagingStubFactory internally and therefor couldn't
    // be mocked. However, this test was already disabled.
    EXPECT_CALL(*messagingStubFactory, create(_)).Times(1);

    messageRouter->route(joynrMessage);

}

TEST_F(MessageRouterTest, addMessageToQueue){
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 2);
}

TEST_F(MessageRouterTest, multicastMessageWillNotBeQueued) {
    joynrMessage.setReceivedFromGlobal(true);
    joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 0);
}

MATCHER_P2(addressWithChannelId, addressType, channelId, "") {
    if (addressType == std::string("mqtt")) {
        auto mqttAddress = std::dynamic_pointer_cast<const system::RoutingTypes::MqttAddress>(arg);
        if (mqttAddress) {
            return mqttAddress->getTopic() == channelId;
        }
        else {
            return false;
        }
    } else if (addressType == std::string("http")) {
        auto httpAddress = std::dynamic_pointer_cast<const system::RoutingTypes::ChannelAddress>(arg);
        if (httpAddress) {
            return httpAddress->getChannelId() == channelId;
        }
        else {
            return false;
        }
    } else {
        return false;
    }
}

TEST_F(MessageRouterTest, doNotAddMessageToQueue){
    joynr::Semaphore semaphore(0);
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    const std::string brokerUri = "brokerUri";
    // this message should be added because no destination header set
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    auto mockMessagingStub = std::make_shared<MockMessagingStub>();

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(brokerUri, testHttp);
    messageRouter->addNextHop(testHttp, httpAddress);
    // the message now has a known destination and should be directly routed
    joynrMessage.setHeaderTo(testHttp);
    EXPECT_CALL(*messagingStubFactory, create(addressWithChannelId("http", testHttp))).Times(1).WillOnce(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillByDefault(ReleaseSemaphore(&semaphore));
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
    EXPECT_CALL(*messagingStubFactory, create(_)).WillRepeatedly(Return(mockMessagingStub));

    // add destination address -> message should be routed
    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    messageRouter->addNextHop(testMqtt, mqttAddress);
    // the message now has a known destination and should be directly routed
    joynrMessage.setHeaderTo(testMqtt);
    EXPECT_CALL(*messagingStubFactory, create(addressWithChannelId("mqtt", testMqtt))).WillRepeatedly(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillByDefault(ReleaseSemaphore(&semaphore));
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TEST_F(MessageRouterTest, resendMessageWhenDestinationAddressIsAdded){
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    const std::string brokerUri = "brokerUri";
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*messagingStubFactory, create(_)).WillByDefault(Return(mockMessagingStub));
    // this message should be added because destination is unknown
    joynrMessage.setHeaderTo(testHttp);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(brokerUri, testHttp);
    messageRouter->addNextHop(testHttp, httpAddress);
    EXPECT_EQ(messageQueue->getQueueLength(), 0);

    joynrMessage.setHeaderTo(testMqtt);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    messageRouter->addNextHop(testMqtt, mqttAddress);
    EXPECT_EQ(messageQueue->getQueueLength(), 0);
}

TEST_F(MessageRouterTest, outdatedMessagesAreRemoved){
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    // we wait for the time out (500ms) and the thread sleep (1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    EXPECT_EQ(messageQueue->getQueueLength(), 0);
}

void MessageRouterTest::routeMessageToAddress() {
    joynr::Semaphore semaphore(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*messagingStubFactory, create(_)).WillByDefault(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
            .WillByDefault(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*mockMessagingStub, transmit(joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()));
    messageRouter->route(joynrMessage);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TEST_F(MessageRouterTest, routeMessageToHttpAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToHttpAddress";
    const std::string destinationChannelId = "TEST_routeMessageToHttpAddress_channelId";
    const std::string messageEndPointUrl = "TEST_messageEndPointUrl";
    auto address = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(messageEndPointUrl, destinationChannelId);

    messageRouter->addNextHop(destinationParticipantId, address);
    joynrMessage.setHeaderTo(destinationParticipantId);

    EXPECT_CALL(*messagingStubFactory,
            create(addressWithChannelId("http", destinationChannelId))
            ).Times(1);

    routeMessageToAddress();
}

TEST_F(MessageRouterTest, routeMessageToMqttAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";
    const std::string destinationChannelId = "TEST_routeMessageToMqttAddress_channelId";
    const std::string brokerUri = "brokerUri";
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, destinationChannelId);

    messageRouter->addNextHop(destinationParticipantId, address);
    joynrMessage.setHeaderTo(destinationParticipantId);

    EXPECT_CALL(*messagingStubFactory,
            create(addressWithChannelId("mqtt", destinationChannelId))
            ).Times(1);

    routeMessageToAddress();
}

TEST_F(MessageRouterTest, restoreRoutingTable) {
    const std::string routingTablePersistenceFilename = "test-RoutingTable.persist";
    std::remove(routingTablePersistenceFilename.c_str());

    auto messagingStubFactory = std::make_shared<MockMessagingStubFactory>();
    auto messageRouter = std::make_unique<MessageRouter>(
                messagingStubFactory,
                std::make_shared<MulticastMessagingSkeletonDirectory>(),
                std::unique_ptr<IPlatformSecurityManager>(),
                singleThreadedIOService.getIOService(),
                nullptr);
    const std::string participantId = "myParticipantId";
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();

    // MUST be done BEFORE savePersistence to pass the persistence filename to the publicationManager
    messageRouter->loadRoutingTable(routingTablePersistenceFilename);
    messageRouter->addProvisionedNextHop(participantId, address); // Saves the RoutingTable to the persistence file.

    messageRouter = std::make_unique<MessageRouter>(
                messagingStubFactory,
                std::make_shared<MulticastMessagingSkeletonDirectory>(),
                std::unique_ptr<IPlatformSecurityManager>(),
                singleThreadedIOService.getIOService(),
                nullptr);

    messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    joynrMessage.setHeaderTo(participantId);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*address)))).Times(1);
    messageRouter->route(joynrMessage);
}

TEST_F(MessageRouterTest, addMulticastReceiver_ChildRouter_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();
    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                                                    singleThreadedIOService.getIOService(),
                                                    nullptr);
    // Set all attributes which are required to make the message router a child message router.
    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    messageRouter->addProvisionedNextHop(providerParticipantId, parentAddress);

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _));

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
}

TEST_F(MessageRouterTest, addMulticastReceiverForWebSocketProvider_ChildRouter_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();
    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                                                    singleThreadedIOService.getIOService(),
                                                    nullptr);
    // Set all attributes which are required to make the message router a child message router.
    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");

    const std::string providerParticipantId("providerParticipantId");
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    EXPECT_CALL(*mockRoutingProxyRef,
        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
            .Times(1)
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiverForInProcessProvider_ChildRouter_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();
    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                                                    singleThreadedIOService.getIOService(),
                                                    nullptr);
    // Set all attributes which are required to make the message router a child message router.
    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");

    const std::string providerParticipantId("providerParticipantId");
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    EXPECT_CALL(*mockRoutingProxyRef,
        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
            .Times(1)
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiverForMqttProvider_NonChildRouter_callsSkeleton) {
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto mockMqttMessagingMulticastSubscriber = std::make_shared<MockMessagingMulticastSubscriber>();
    std::shared_ptr<IMessagingMulticastSubscriber> mqttMessagingMulticastSubscriber = mockMqttMessagingMulticastSubscriber;

    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
                mqttMessagingMulticastSubscriber);

    EXPECT_CALL(*mockMqttMessagingMulticastSubscriber, registerMulticastSubscription(multicastId));

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiverForHttpProvider_NonChildRouter_callsSkeleton) {
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();

    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::ChannelAddress>(
                skeleton);

    EXPECT_CALL(*skeleton, registerMulticastSubscription(multicastId));

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiverForWebSocketProvider_NonChildRouter_succeeds) {
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiverForInProcessProvider_NonChildRouter_succeeds) {
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();

    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiver_NonChildRouter_failsIfProviderAddressNotAvailable) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    Semaphore errorCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { FAIL() << "onSuccess called"; },
        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
        {
            errorCallbackCalled.notify();
        });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiver_ChildRouter_callsParentRouterIfProviderAddressNotAvailable) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    EXPECT_CALL(*mockRoutingProxy, resolveNextHopAsync(providerParticipantId,_,_))
            .WillOnce(
                DoAll(
                    InvokeArgument<2>(joynr::exceptions::JoynrRuntimeException("testException")),
                    Return(nullptr)))
            .WillOnce(DoAll(InvokeArgument<1>(false), Return(nullptr)))
            .WillOnce(DoAll(InvokeArgument<1>(true), Return(nullptr)));

    EXPECT_CALL(*mockRoutingProxy, addMulticastReceiverAsync(
                    multicastId,
                    subscriberParticipantId,
                    providerParticipantId,
                    _,
                    _))
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );;

    messageRouter = std::make_unique<MessageRouter>(
                std::move(messagingStubFactory),
                std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                singleThreadedIOService.getIOService(),
                nullptr);
    // Set all attributes which are required to make the message router a child message router.
    messageRouter->setParentRouter(
                std::move(mockRoutingProxy),
                parentAddress,
                std::string("parentParticipantId"));

    Semaphore errorCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { FAIL() << "onSuccess called"; },
        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
        {
            errorCallbackCalled.notify();
        });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { FAIL() << "onSuccess called"; },
        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
        {
            errorCallbackCalled.notify();
        });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    bool hasNextHop = false;
    messageRouter->resolveNextHop(
                providerParticipantId,
                [&hasNextHop](const bool& resolved) { hasNextHop = resolved; },
                nullptr);
    EXPECT_FALSE(hasNextHop);

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    messageRouter->resolveNextHop(
                providerParticipantId,
                [&successCallbackCalled](const bool& resolved) {
                    if (resolved) {
                        successCallbackCalled.notify();
                    } else {
                        FAIL() << "resolveNextHop failed";
                    }
                },
                nullptr); // not called in resolveNextHop
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, addMulticastReceiver_NonChildRouter_succeedsIfSkeletonNotAvailable) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException& exception)
        {
            FAIL() << "onError called: " << exception.what();
        });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, removeMulticastReceiver_ChildRouter_CallsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();
    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                                                    singleThreadedIOService.getIOService(),
                                                    nullptr);

    // Set all attributes which are required to make the message router a child message router.
    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    messageRouter->addProvisionedNextHop(providerParticipantId, parentAddress);

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
        removeMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _));

    messageRouter->removeMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
}

TEST_F(MessageRouterTest, removeMulticastReceiverOfInProcessProvider_ChildRouter_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();
    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();

    messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
                                                    std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                                                    singleThreadedIOService.getIOService(),
                                                    nullptr);

    // Set all attributes which are required to make the message router a child message router.
    messageRouter->setParentRouter(std::move(mockRoutingProxy), parentAddress, std::string("parentParticipantId"));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    EXPECT_CALL(*mockRoutingProxyRef,
        removeMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
            .Times(1)
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, removeMulticastReceiver_failsIfProviderAddressNotAvailable) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::WebSocketClientAddress>(
                skeleton);

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    messageRouter->removeNextHop(providerParticipantId);

    Semaphore errorCallbackCalled;
    messageRouter->removeMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { FAIL() << "onSuccess called"; },
        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&){ errorCallbackCalled.notify(); });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, removeMulticastReceiver_NonChildRouter_succeedsIfSkeletonNotAvailable) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
                skeleton);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    multicastMessagingSkeletonDirectory->unregisterSkeleton<system::RoutingTypes::MqttAddress>();

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, removeMulticastReceiverOfStandaloneProvider_NonChildRouter_callsSkeleton) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();

    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();

    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
                skeleton);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    // The message router shall unregister the subscription at the multicast skeleton
    EXPECT_CALL(*skeleton, unregisterMulticastSubscription(multicastId)).Times(1);

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, removeMulticastReceiverOfWebSocketProvider_NonChildRouter_succeeds) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, removeMulticastReceiverOfInProcessProvider_NonChildRouter_succeeds) {
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();

    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(
        multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(MessageRouterTest, routeMulticastMessageFromWebSocketProvider_multicastMsgIsSentToAllMulticastRecivers) {
    const std::string subscriberParticipantId1("subscriberPartId1");
    const std::string subscriberParticipantId2("subscriberPartId2");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);

    // create a new pointer representin the multicast address
    std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> multicastAddress(
                new joynr::system::RoutingTypes::MqttAddress(*globalTransport)
    );
    multicastAddress->setTopic(multicastId);

    auto expectedAddress1 = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>("subscriberTopic1");
    auto expectedAddress2 = std::make_shared<const joynr::InProcessMessagingAddress>();
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>("providerTopic");

    messageRouter->addProvisionedNextHop(subscriberParticipantId1, expectedAddress1);
    messageRouter->addProvisionedNextHop(subscriberParticipantId2, expectedAddress2);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::WebSocketClientAddress>(
                skeleton);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId1,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId2,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    joynrMessage.setHeaderFrom(providerParticipantId);
    joynrMessage.setHeaderTo(multicastId);

    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress1)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress2)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*multicastAddress)))).Times(1);

    messageRouter->route(joynrMessage);
}

TEST_F(MessageRouterTest, routeMulticastMessageFromInProcessProvider_multicastMsgIsSentToAllMulticastRecivers) {
    const std::string subscriberParticipantId1("subscriberPartId1");
    const std::string subscriberParticipantId2("subscriberPartId2");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);

    // create a new pointer representin the multicast address
    std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> multicastAddress(
                new joynr::system::RoutingTypes::MqttAddress(*globalTransport)
    );
    multicastAddress->setTopic(multicastId);

    auto expectedAddress1 = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>("subscriberTopic1");
    auto expectedAddress2 = std::make_shared<const joynr::InProcessMessagingAddress>();
    auto mockInProcessMessagingSkeleton = std::make_shared<MockInProcessMessagingSkeleton>();
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(mockInProcessMessagingSkeleton);

    messageRouter->addProvisionedNextHop(subscriberParticipantId1, expectedAddress1);
    messageRouter->addProvisionedNextHop(subscriberParticipantId2, expectedAddress2);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId1,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId2,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    joynrMessage.setHeaderFrom(providerParticipantId);
    joynrMessage.setHeaderTo(multicastId);

    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress1)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress2)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*multicastAddress)))).Times(1);

    messageRouter->route(joynrMessage);
}

TEST_F(MessageRouterTest, routeMulticastMessageFromGlobalProvider_multicastMsgIsSentToAllMulticastRecivers) {
    const std::string subscriberParticipantId1("subscriberPartId1");
    const std::string subscriberParticipantId2("subscriberPartId2");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);

    // create a new pointer representin the multicast address
    std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> multicastAddress(
                new joynr::system::RoutingTypes::MqttAddress(*globalTransport)
    );
    multicastAddress->setTopic(multicastId);

    auto expectedAddress1 = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>("subscriberTopic1");
    auto expectedAddress2 = std::make_shared<const joynr::InProcessMessagingAddress>();
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>("brokerUri", "providerTopic");

    messageRouter->addProvisionedNextHop(subscriberParticipantId1, expectedAddress1);
    messageRouter->addProvisionedNextHop(subscriberParticipantId2, expectedAddress2);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
                skeleton);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId1,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId2,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    joynrMessage.setHeaderFrom(providerParticipantId);
    joynrMessage.setHeaderTo(multicastId);
    joynrMessage.setReceivedFromGlobal(true);

    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress1)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress2)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*multicastAddress)))).Times(0);

    messageRouter->route(joynrMessage);
}

template <typename T>
bool IsAddressOfType(const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address) {
    if (std::dynamic_pointer_cast<T>(address)) {
        return true;
    } else {
        return false;
    }
}

TEST_F(MessageRouterTest, routeMulticastMessageFromGlobalProvider_multicastMsgIsNotPublishedGlobally) {
    const std::string providerParticipantIdMqtt("providerParticipantIdMqtt");
    const std::string providerParticipantIdHttp("providerParticipantIdHttp");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastIdMqttProvider(providerParticipantIdMqtt + "/" + multicastNameAndPartitions);
    const std::string multicastIdHttpProvider(providerParticipantIdHttp + "/" + multicastNameAndPartitions);
    auto providerAddressMqtt = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
                "brokerUri", "providerTopic");
    auto providerAddressHttp = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                "brokerUri", "providerChannelId");
    messageRouter->addProvisionedNextHop(providerParticipantIdMqtt, providerAddressMqtt);
    messageRouter->addProvisionedNextHop(providerParticipantIdHttp, providerAddressHttp);

    const std::string subscriberParticipantIdMqtt("subscriberPartIdMqtt");
    const std::string subscriberParticipantIdHttp("subscriberPartIdHttp");
    auto subscriberAddressMqtt = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>("subscriberMqtt");
    auto subscriberAddressHttp = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>("subscriberHttp");
    messageRouter->addProvisionedNextHop(subscriberParticipantIdMqtt, subscriberAddressMqtt);
    messageRouter->addProvisionedNextHop(subscriberParticipantIdHttp, subscriberAddressHttp);
    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
                skeleton);
    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::ChannelAddress>(
                skeleton);

    messageRouter->addMulticastReceiver(
        multicastIdMqttProvider,
        subscriberParticipantIdMqtt,
        providerParticipantIdMqtt,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    messageRouter->addMulticastReceiver(
        multicastIdHttpProvider,
        subscriberParticipantIdHttp,
        providerParticipantIdHttp,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    EXPECT_CALL(
            *messagingStubFactory,
            create(Truly(IsAddressOfType<const system::RoutingTypes::WebSocketClientAddress>))
    ).Times(2);

    EXPECT_CALL(
            *messagingStubFactory,
            create(Truly(IsAddressOfType<const system::RoutingTypes::MqttAddress>))
    ).Times(0);

    EXPECT_CALL(
            *messagingStubFactory,
            create(Truly(IsAddressOfType<const system::RoutingTypes::ChannelAddress>))
     ).Times(0);

    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    joynrMessage.setReceivedFromGlobal(true);

    joynrMessage.setHeaderFrom(providerParticipantIdMqtt);
    joynrMessage.setHeaderTo(multicastIdMqttProvider);
    messageRouter->route(joynrMessage);

    joynrMessage.setHeaderFrom(providerParticipantIdHttp);
    joynrMessage.setHeaderTo(multicastIdHttpProvider);
    messageRouter->route(joynrMessage);
}

TEST_F(MessageRouterTest, routeMulticastMessageFromLocalProvider_multicastMsgIsSentToAllMulticastReceivers) {

    MockRoutingProxy *mockRoutingProxy = new MockRoutingProxy();

    ON_CALL(
        *mockRoutingProxy,
        addMulticastReceiverAsync(_,_,_,_,_)
    ).WillByDefault(
        DoAll(
            InvokeArgument<3>(),
            Return(nullptr)
        )
    );

    auto parentAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
                joynr::system::RoutingTypes::WebSocketProtocol::Enum::WS,
                "host",
                4242,
                "path"
    );

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
            std::make_unique<WebSocketMulticastAddressCalculator>(parentAddress);

    messageRouter = std::make_unique<MessageRouter>(
                messagingStubFactory,
                std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(),
                singleThreadedIOService.getIOService(),
                std::move(addressCalculator)
    );

    messageRouter->setParentRouter(
                std::unique_ptr<MockRoutingProxy>(mockRoutingProxy),
                parentAddress,
                std::string("parentParticipantId")
    );

    const std::string subscriberParticipantId1("subscriberPartId");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
    const std::shared_ptr<const joynr::InProcessMessagingAddress> expectedAddress =
            std::make_shared<const joynr::InProcessMessagingAddress>();

    messageRouter->addProvisionedNextHop(providerParticipantId, parentAddress);
    messageRouter->addProvisionedNextHop(subscriberParticipantId1, expectedAddress);

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId1,
        providerParticipantId,
        [](){ },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*parentAddress)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*expectedAddress)))).Times(1);

    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    joynrMessage.setHeaderFrom(providerParticipantId);
    joynrMessage.setHeaderTo(multicastId);

    messageRouter->route(joynrMessage);
}

TEST_F(MessageRouterTest, loadNonExistingMulticastDirectoryPersistenceFile) {
    EXPECT_NO_THROW(messageRouter->loadMulticastReceiverDirectory("not-existing.persist"));
}

TEST_F(MessageRouterTest, persistMulticastReceiverDirectory) {
    const std::string providerParticipantId("providerParticipantId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string multicastId = util::createMulticastId(providerParticipantId, "multicastName", {});

    auto providerAddress = std::make_shared<system::RoutingTypes::MqttAddress>();
    auto multicastSubscriber = std::make_shared<MockMessagingMulticastSubscriber>();

    const std::string persistencyFilename = "multicast-receiver-directory-test.persist";
    std::remove(persistencyFilename.c_str());
    // Load method stores the filename which will later be used to save the multicast receiver directory.
    messageRouter->loadMulticastReceiverDirectory(persistencyFilename);

    multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(multicastSubscriber);
    messageRouter->addNextHop(providerParticipantId, providerAddress);
    messageRouter->addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId, []() {}, nullptr);

    createNewMessageRouter();
    messageRouter->addNextHop(providerParticipantId, providerAddress);

    EXPECT_CALL(*multicastSubscriber, registerMulticastSubscription(multicastId)).Times(1);
    messageRouter->loadMulticastReceiverDirectory(persistencyFilename);
}

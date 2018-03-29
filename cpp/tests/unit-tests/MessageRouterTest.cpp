/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "tests/unit-tests/MessageRouterTest.h"

#include <cstdint>
#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Semaphore.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MessageQueue.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/IPlatformSecurityManager.h"

#include "tests/mock/MockTransportStatus.h"

using ::testing::InvokeArgument;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::Truly;
using ::testing::SaveArg;
using ::testing::A;
using ::testing::_;
using ::testing::Eq;

using namespace joynr;

typedef ::testing::Types<
        LibJoynrMessageRouter,
        CcMessageRouter
    > MessageRouterTypes;

TYPED_TEST_CASE(MessageRouterTest, MessageRouterTypes);

TYPED_TEST(MessageRouterTest, addMessageToQueue){
    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();

    this->messageRouter->route(immutableMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    this->messageRouter->route(immutableMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 2);
}

TYPED_TEST(MessageRouterTest, multicastMessageWillNotBeQueued) {
    this->mutableMessage.setType(Message::VALUE_MESSAGE_TYPE_MULTICAST());
    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();
    immutableMessage->setReceivedFromGlobal(true);
    this->messageRouter->route(immutableMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
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

TYPED_TEST(MessageRouterTest, doNotAddMessageToQueue){
    joynr::Semaphore semaphore(0);
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    const std::string brokerUri = "brokerUri";

    std::shared_ptr<ImmutableMessage> immutableMessage1 = this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage1);

    // this message should be added to the queue because no destination header set
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    auto mockMessagingStub = std::make_shared<MockMessagingStub>();

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(brokerUri, testHttp);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;
    this->messageRouter->addNextHop(testHttp, httpAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    // the message now has a known destination and should be directly routed
    this->mutableMessage.setRecipient(testHttp);
    std::shared_ptr<ImmutableMessage> immutableMessage2 = this->mutableMessage.getImmutableMessage();
    EXPECT_CALL(*(this->messagingStubFactory), create(addressWithChannelId("http", testHttp))).Times(1).WillOnce(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(immutableMessage2, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillByDefault(ReleaseSemaphore(&semaphore));
    this->messageRouter->route(immutableMessage2);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
    EXPECT_CALL(*(this->messagingStubFactory), create(_)).WillRepeatedly(Return(mockMessagingStub));

    // add destination address -> message should be routed
    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    this->messageRouter->addNextHop(testMqtt, mqttAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    // the message now has a known destination and should be directly routed
    this->mutableMessage.setRecipient(testMqtt);
    std::shared_ptr<ImmutableMessage> immutableMessage3 = this->mutableMessage.getImmutableMessage();
    EXPECT_CALL(*(this->messagingStubFactory), create(addressWithChannelId("mqtt", testMqtt))).WillRepeatedly(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(immutableMessage3, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillByDefault(ReleaseSemaphore(&semaphore));
    this->messageRouter->route(immutableMessage3);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TYPED_TEST(MessageRouterTest, resendMessageWhenDestinationAddressIsAdded){
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    const std::string brokerUri = "brokerUri";
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*(this->messagingStubFactory), create(_)).WillByDefault(Return(mockMessagingStub));
    this->mutableMessage.setRecipient(testHttp);

    std::shared_ptr<ImmutableMessage> immutableMessage1 = this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage1);

    // this message should be added to the queue because destination is unknown
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(brokerUri, testHttp);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;
    this->messageRouter->addNextHop(testHttp, httpAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);

    this->mutableMessage.setRecipient(testMqtt);
    std::shared_ptr<ImmutableMessage> immutableMessage2 = this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage2);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    this->messageRouter->addNextHop(testMqtt, mqttAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
}

TYPED_TEST(MessageRouterTest, outdatedMessagesAreRemoved){
    this->messageRouter->route(this->mutableMessage.getImmutableMessage());
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    // we wait for the time out (500ms) and the thread sleep (1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
}

TYPED_TEST(MessageRouterTest, routeMessageToHttpAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToHttpAddress";
    const std::string destinationChannelId = "TEST_routeMessageToHttpAddress_channelId";
    const std::string messageEndPointUrl = "TEST_messageEndPointUrl";
    auto address = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(messageEndPointUrl, destinationChannelId);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;

    this->messageRouter->addNextHop(destinationParticipantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    this->mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->messagingStubFactory),
            create(addressWithChannelId("http", destinationChannelId))
            ).Times(1);

    this->routeMessageToAddress();
}

TYPED_TEST(MessageRouterTest, routeMessageToMqttAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";
    const std::string destinationChannelId = "TEST_routeMessageToMqttAddress_channelId";
    const std::string brokerUri = "brokerUri";
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, destinationChannelId);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;

    this->messageRouter->addNextHop(destinationParticipantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    this->mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->messagingStubFactory),
            create(addressWithChannelId("mqtt", destinationChannelId))
            ).Times(1);

    this->routeMessageToAddress();
}

template <class T>
void MessageRouterTest<T>::checkAllowUpdate(bool allowUpdate, bool updateExpected){
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";
    const std::string oldDestinationChannelId = "TEST_routeMessageToMqttAddress_old_channelId";
    const std::string newDestinationChannelId = "TEST_routeMessageToMqttAddress_new_channelId";
    const std::string brokerUri = "brokerUri";
    auto oldAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, oldDestinationChannelId);
    auto newAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, newDestinationChannelId);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    this->messageRouter->addNextHop(destinationParticipantId, oldAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    this->messageRouter->addNextHop(destinationParticipantId, newAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    this->mutableMessage.setRecipient(destinationParticipantId);

    auto expectedAddress = updateExpected ? newDestinationChannelId : oldDestinationChannelId;
    EXPECT_CALL(*(this->messagingStubFactory),
            create(addressWithChannelId("mqtt", expectedAddress))
            ).Times(1);

    this->routeMessageToAddress();
}

TYPED_TEST(MessageRouterTest, routedMessageQueuedIfTransportIsNotAvailable) {
    auto mockTransportStatus = std::make_shared<MockTransportStatus>();

    std::function<void(bool)> availabilityChangedCallback;
    EXPECT_CALL(*mockTransportStatus, setAvailabilityChangedCallback(_)).WillOnce(SaveArg<0>(&availabilityChangedCallback));

    this->messageRouter->shutdown();
    this->messageRouter = this->createMessageRouter({mockTransportStatus});

    const std::string to = "to";
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;
    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    auto address = std::dynamic_pointer_cast<const joynr::system::RoutingTypes::Address>(mqttAddress);

    this->messageRouter->addNextHop(to, mqttAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
    this->mutableMessage.setRecipient(to);

    ON_CALL(*mockTransportStatus, isReponsibleFor(address)).
            WillByDefault(Return(true));
    EXPECT_CALL(*mockTransportStatus, isAvailable()).
            Times(3).
            WillOnce(Return(false)).
            WillOnce(Return(false)).
            WillOnce(Return(true));

    // The first message is supposed to be queued as the transport is not available
    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage);

    EXPECT_EQ(1, this->transportNotAvailableQueueRef->getQueueLength());

    // Now pretend that the transport became available
    joynr::Semaphore semaphore(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*mockMessagingStub, transmit(immutableMessage, _)).
            WillByDefault(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*(this->messagingStubFactory), create(address)).
            Times(1).
            WillOnce(Return(mockMessagingStub));

    availabilityChangedCallback(true);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TYPED_TEST(MessageRouterTest, restoreRoutingTable) {
    const std::string participantId = "myParticipantId";
    const std::string routingTablePersistenceFilename = "test-RoutingTable.persist";
    std::remove(routingTablePersistenceFilename.c_str());

    // Load and set RoutingTable persistence filename
    this->messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    const bool isGloballyVisible = true;
    this->messageRouter->addProvisionedNextHop(participantId, address, isGloballyVisible); // Saves routingTable to the persistence file.

    // create a new MessageRouter
    this->messageRouter->shutdown();
    this->messageRouter = this->createMessageRouter();
    this->messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    this->mutableMessage.setRecipient(participantId);
    EXPECT_CALL(*(this->messagingStubFactory),
                create(Pointee(Eq(*address)))).Times(1);
    this->messageRouter->route(this->mutableMessage.getImmutableMessage());
}

TYPED_TEST(MessageRouterTest, cleanupExpiredMessagesFromTransportNotAvailableQueue) {
    auto mockTransportStatus  = std::make_shared<MockTransportStatus>();
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    auto address = std::dynamic_pointer_cast<const joynr::system::RoutingTypes::Address>(providerAddress);
    const std::string providerParticipantId("providerParticipantId");

    this->mutableMessage.setRecipient(providerParticipantId);
    const bool isGloballyVisible = true;

    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;
    transportStatuses.emplace_back(mockTransportStatus);

    // create a new MessageRouter
    this->messageRouter->shutdown();
    this->messageRouter = this->createMessageRouter({transportStatuses});
    this->messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress, isGloballyVisible); // Saves routingTable to the persistence file.

    ON_CALL(*mockTransportStatus, isReponsibleFor(address)).
            WillByDefault(Return(true));

    std::shared_ptr<ImmutableMessage> immutableMessage1 = this->mutableMessage.getImmutableMessage();

    this->mutableMessage.setExpiryDate(TimePoint::fromRelativeMs(10000));
    std::shared_ptr<ImmutableMessage> immutableMessage2 = this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage1);
    this->messageRouter->route(immutableMessage2);
    EXPECT_EQ(this->transportNotAvailableQueueRef->getQueueLength(), 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(5500));
    EXPECT_EQ(this->transportNotAvailableQueueRef->getQueueLength(), 1); // it remains immutableMessage2
    auto queuedMessage2 = this->transportNotAvailableQueueRef->getNextMessageFor(mockTransportStatus);
    EXPECT_EQ(queuedMessage2, immutableMessage2);
}

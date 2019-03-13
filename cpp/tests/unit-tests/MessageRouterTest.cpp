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

#include <chrono>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Semaphore.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MessageQueue.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/IPlatformSecurityManager.h"

#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockInProcessMessagingSkeleton.h"
#include "tests/mock/MockTransportStatus.h"

using ::testing::_;
using ::testing::A;
using ::testing::Action;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::InvokeArgument;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Truly;

using namespace joynr;

typedef ::testing::Types<LibJoynrMessageRouter, CcMessageRouter> MessageRouterTypes;

TYPED_TEST_CASE(MessageRouterTest, MessageRouterTypes);

TYPED_TEST(MessageRouterTest, addMessageToQueue)
{
    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();

    EXPECT_EQ(this->messageRouter->getNumberOfRoutedMessages(), 0);

    this->messageRouter->route(immutableMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    this->messageRouter->route(immutableMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 2);

    EXPECT_EQ(this->messageRouter->getNumberOfRoutedMessages(), 2);
}

TYPED_TEST(MessageRouterTest, multicastMessageWillNotBeQueued)
{
    this->mutableMessage.setType(Message::VALUE_MESSAGE_TYPE_MULTICAST());
    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();
    immutableMessage->setReceivedFromGlobal(true);
    this->messageRouter->route(immutableMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
}

MATCHER_P(addressWithSkeleton, skeleton, "")
{
    auto inProcessAddress = std::dynamic_pointer_cast<const InProcessMessagingAddress>(arg);
    if (inProcessAddress) {
        return inProcessAddress->getSkeleton().get() == skeleton.get();
    } else {
        return false;
    }
}

TYPED_TEST(MessageRouterTest, doNotAddMessageToQueue)
{
    joynr::Semaphore semaphore(0);
    const std::string unknownParticipantId = "unknownParticipantId";
    const std::string knownParticipantId = "knownParticipantId";

    this->mutableMessage.setRecipient(unknownParticipantId);
    std::shared_ptr<ImmutableMessage> immutableMessage1 =
            this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage1);

    // this message should be added to the queue because no destination header set
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    auto mockMessagingStub = std::make_shared<MockMessagingStub>();

    // add destination address -> message should be routed
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>(skeleton);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    this->messageRouter->addNextHop(
            knownParticipantId, inProcessAddress, isGloballyVisible, expiryDateMs, isSticky);

    // the message now has a known destination and should be directly routed
    this->mutableMessage.setRecipient(knownParticipantId);
    std::shared_ptr<ImmutableMessage> immutableMessage2 =
            this->mutableMessage.getImmutableMessage();
    EXPECT_CALL(*(this->messagingStubFactory), create(addressWithSkeleton(skeleton)))
            .Times(1)
            .WillOnce(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub,
            transmit(immutableMessage2,
                     A<const std::function<
                             void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
            .WillByDefault(ReleaseSemaphore(&semaphore));
    this->messageRouter->route(immutableMessage2);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TYPED_TEST(MessageRouterTest, resendMessageWhenDestinationAddressIsAdded)
{
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    const std::string brokerUri = "brokerUri";
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*(this->messagingStubFactory), create(_)).WillByDefault(Return(mockMessagingStub));
    this->mutableMessage.setRecipient(testHttp);

    std::shared_ptr<ImmutableMessage> immutableMessage1 =
            this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage1);

    // this message should be added to the queue because destination is unknown
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
            brokerUri, testHttp);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    this->messageRouter->addNextHop(
            testHttp, httpAddress, isGloballyVisible, expiryDateMs, isSticky);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);

    this->mutableMessage.setRecipient(testMqtt);
    std::shared_ptr<ImmutableMessage> immutableMessage2 =
            this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage2);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    auto mqttAddress =
            std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    this->messageRouter->addNextHop(
            testMqtt, mqttAddress, isGloballyVisible, expiryDateMs, isSticky);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
}

TYPED_TEST(MessageRouterTest, outdatedMessagesAreRemoved)
{
    this->messageRouter->route(this->mutableMessage.getImmutableMessage());
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    // we wait for the time out (500ms) and the thread sleep (1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
}

TYPED_TEST(MessageRouterTest, routeMessageToInProcessAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToInProcessAddress";
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>(skeleton);

    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    this->messageRouter->addNextHop(destinationParticipantId,
                                    inProcessAddress,
                                    isGloballyVisible,
                                    expiryDateMs,
                                    isSticky);
    this->mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->messagingStubFactory),
                create(addressWithSkeleton(skeleton))).Times(1);

    this->routeMessageToAddress();
}

TYPED_TEST(MessageRouterTest, routedMessageQueuedIfTransportIsNotAvailable)
{
    auto mockTransportStatus = std::make_shared<MockTransportStatus>();

    std::function<void(bool)> availabilityChangedCallback;
    EXPECT_CALL(*mockTransportStatus, setAvailabilityChangedCallback(_))
            .WillOnce(SaveArg<0>(&availabilityChangedCallback));

    this->messageRouter->shutdown();
    this->messageRouter = this->createMessageRouter({mockTransportStatus});

    const std::string to = "to";
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>(skeleton);
    auto address =
            std::dynamic_pointer_cast<const joynr::system::RoutingTypes::Address>(inProcessAddress);

    this->messageRouter->addNextHop(
            to, inProcessAddress, isGloballyVisible, expiryDateMs, isSticky);
    this->mutableMessage.setRecipient(to);

    ON_CALL(*mockTransportStatus, isReponsibleFor(address)).WillByDefault(Return(true));
    EXPECT_CALL(*mockTransportStatus, isAvailable())
            .Times(3)
            .WillOnce(Return(false))
            .WillOnce(Return(false))
            .WillOnce(Return(true));

    // The first message is supposed to be queued as the transport is not available
    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage);

    EXPECT_EQ(1, this->transportNotAvailableQueueRef->getQueueLength());

    // Now pretend that the transport became available
    joynr::Semaphore semaphore(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*mockMessagingStub, transmit(immutableMessage, _))
            .WillByDefault(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*(this->messagingStubFactory), create(addressWithSkeleton(skeleton))).Times(1).WillOnce(
            Return(mockMessagingStub));

    availabilityChangedCallback(true);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TYPED_TEST(MessageRouterTest, restoreRoutingTable)
{
    Semaphore semaphore(0);
    bool isLibJoynr = false;
    const std::string participantId = "myParticipantId";
    const std::string routingTablePersistenceFilename = "test-RoutingTable.persist";
    std::remove(routingTablePersistenceFilename.c_str());

    // Load and set RoutingTable persistence filename
    this->messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    auto webSocketAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();
    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    if (typeid(*this->messageRouter) == typeid(LibJoynrMessageRouter)) {
        isLibJoynr = true;
    }
    const bool isGloballyVisible = true;
    if (isLibJoynr) {
        this->messageRouter->addProvisionedNextHop(
                participantId,
                webSocketAddress,
                isGloballyVisible); // Saves routingTable to the persistence file.
    } else {
        this->messageRouter->addProvisionedNextHop(
                participantId,
                mqttAddress,
                isGloballyVisible); // Saves routingTable to the persistence file.
    }

    // create a new MessageRouter
    this->messageRouter->shutdown();
    this->messageRouter = this->createMessageRouter();
    this->messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    this->mutableMessage.setRecipient(participantId);
    std::shared_ptr<IMessagingStub> mockMessagingStub = std::make_shared<MockMessagingStub>();
    if (isLibJoynr) {
        EXPECT_CALL(*(this->messagingStubFactory), create(Pointee(Eq(*webSocketAddress))))
                .WillOnce(DoAll(ReleaseSemaphore(&semaphore), Return(mockMessagingStub)));
    } else {
        EXPECT_CALL(*(this->messagingStubFactory), create(Pointee(Eq(*mqttAddress))))
                .WillOnce(DoAll(ReleaseSemaphore(&semaphore), Return(mockMessagingStub)));
    }
    this->messageRouter->route(this->mutableMessage.getImmutableMessage());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
}

TYPED_TEST(MessageRouterTest, cleanupExpiredMessagesFromTransportNotAvailableQueue)
{
    auto mockTransportStatus = std::make_shared<MockTransportStatus>();
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto providerAddress = std::make_shared<const InProcessMessagingAddress>(skeleton);
    auto address =
            std::dynamic_pointer_cast<const joynr::system::RoutingTypes::Address>(providerAddress);
    const std::string providerParticipantId("providerParticipantId");

    this->mutableMessage.setRecipient(providerParticipantId);
    const bool isGloballyVisible = true;

    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;
    transportStatuses.emplace_back(mockTransportStatus);

    // create a new MessageRouter
    this->messageRouter->shutdown();
    this->messageRouter = this->createMessageRouter({transportStatuses});
    this->messageRouter->addProvisionedNextHop(
            providerParticipantId,
            providerAddress,
            isGloballyVisible); // Saves routingTable to the persistence file.

    ON_CALL(*mockTransportStatus, isReponsibleFor(address)).WillByDefault(Return(true));

    std::shared_ptr<ImmutableMessage> immutableMessage1 =
            this->mutableMessage.getImmutableMessage();

    this->mutableMessage.setExpiryDate(TimePoint::fromRelativeMs(10000));
    std::shared_ptr<ImmutableMessage> immutableMessage2 =
            this->mutableMessage.getImmutableMessage();
    this->messageRouter->route(immutableMessage1);
    this->messageRouter->route(immutableMessage2);
    EXPECT_EQ(this->transportNotAvailableQueueRef->getQueueLength(), 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(5500));
    EXPECT_EQ(this->transportNotAvailableQueueRef->getQueueLength(),
              1); // it remains immutableMessage2
    auto queuedMessage2 =
            this->transportNotAvailableQueueRef->getNextMessageFor(mockTransportStatus);
    EXPECT_EQ(queuedMessage2, immutableMessage2);
}

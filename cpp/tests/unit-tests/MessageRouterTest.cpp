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

#include <cstdint>
#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MessageRouterTest.h"

#include "joynr/Semaphore.h"
#include "tests/utils/MockObjects.h"
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

using ::testing::InvokeArgument;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::Truly;

using namespace joynr;

typedef ::testing::Types<
        LibJoynrMessageRouter,
        CcMessageRouter
    > MessageRouterTypes;

TYPED_TEST_CASE(MessageRouterTest, MessageRouterTypes);

TYPED_TEST(MessageRouterTest, addMessageToQueue){
    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 2);
}

TYPED_TEST(MessageRouterTest, multicastMessageWillNotBeQueued) {
    this->joynrMessage.setReceivedFromGlobal(true);
    this->joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    this->messageRouter->route(this->joynrMessage);
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

    // this message should be added because no destination header set
    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    auto mockMessagingStub = std::make_shared<MockMessagingStub>();

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(brokerUri, testHttp);
    const bool isGloballyVisible = true;
    this->messageRouter->addNextHop(testHttp, httpAddress, isGloballyVisible);
    // the message now has a known destination and should be directly routed
    this->joynrMessage.setHeaderTo(testHttp);
    EXPECT_CALL(*(this->messagingStubFactory), create(addressWithChannelId("http", testHttp))).Times(1).WillOnce(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(this->joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillByDefault(ReleaseSemaphore(&semaphore));
    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
    EXPECT_CALL(*(this->messagingStubFactory), create(_)).WillRepeatedly(Return(mockMessagingStub));

    // add destination address -> message should be routed
    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    this->messageRouter->addNextHop(testMqtt, mqttAddress, isGloballyVisible);
    // the message now has a known destination and should be directly routed
    this->joynrMessage.setHeaderTo(testMqtt);
    EXPECT_CALL(*(this->messagingStubFactory), create(addressWithChannelId("mqtt", testMqtt))).WillRepeatedly(Return(mockMessagingStub));
    ON_CALL(*mockMessagingStub, transmit(this->joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillByDefault(ReleaseSemaphore(&semaphore));
    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}

TYPED_TEST(MessageRouterTest, resendMessageWhenDestinationAddressIsAdded){
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    const std::string brokerUri = "brokerUri";
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*(this->messagingStubFactory), create(_)).WillByDefault(Return(mockMessagingStub));
    // this message should be added because destination is unknown
    this->joynrMessage.setHeaderTo(testHttp);
    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(brokerUri, testHttp);
    const bool isGloballyVisible = true;
    this->messageRouter->addNextHop(testHttp, httpAddress, isGloballyVisible);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);

    this->joynrMessage.setHeaderTo(testMqtt);
    this->messageRouter->route(this->joynrMessage);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 1);

    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    this->messageRouter->addNextHop(testMqtt, mqttAddress, isGloballyVisible);
    EXPECT_EQ(this->messageQueue->getQueueLength(), 0);
}

TYPED_TEST(MessageRouterTest, outdatedMessagesAreRemoved){
    this->messageRouter->route(this->joynrMessage);
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

    this->messageRouter->addNextHop(destinationParticipantId, address, isGloballyVisible);
    this->joynrMessage.setHeaderTo(destinationParticipantId);

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

    this->messageRouter->addNextHop(destinationParticipantId, address, isGloballyVisible);
    this->joynrMessage.setHeaderTo(destinationParticipantId);

    EXPECT_CALL(*(this->messagingStubFactory),
            create(addressWithChannelId("mqtt", destinationChannelId))
            ).Times(1);

    this->routeMessageToAddress();
}

TYPED_TEST(MessageRouterTest, restoreRoutingTable) {
    const std::string participantId = "myParticipantId";
    const std::string routingTablePersistenceFilename = "test-RoutingTable.persist";
    std::remove(routingTablePersistenceFilename.c_str());

    // Load and set RoutingTable persistence filename
    this->messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    this->messageRouter->addProvisionedNextHop(participantId, address); // Saves routingTable to the persistence file.

    // create a new MessageRouter
    this->messageRouter = this->createMessageRouter();
    this->messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    this->joynrMessage.setHeaderTo(participantId);
    EXPECT_CALL(*(this->messagingStubFactory), 
                create(Pointee(Eq(*address)))).Times(1);
    this->messageRouter->route(this->joynrMessage);
}


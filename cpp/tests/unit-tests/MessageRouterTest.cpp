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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/MessageRouter.h"
#include "tests/utils/MockObjects.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MessageQueue.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include <chrono>
#include <cstdint>

using ::testing::AllOf;
using ::testing::Property;
using ::testing::Return;
using ::testing::WhenDynamicCastTo;
using namespace joynr;

class MessageRouterTest : public ::testing::Test {
public:
    MessageRouterTest() :
        settings(),
        messagingSettings(settings),
        messagingStubFactory(new MockMessagingStubFactory()),
        messageQueue(new MessageQueue()),
        messageRouter(new MessageRouter(messagingStubFactory, nullptr, 6, messageQueue)),
        joynrMessage()
    {
        // provision global capabilities directory
        std::shared_ptr<joynr::system::RoutingTypes::Address> addressCapabilitiesDirectory(
            new system::RoutingTypes::ChannelAddress(
                        messagingSettings.getCapabilitiesDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);
        // provision channel url directory
        std::shared_ptr<joynr::system::RoutingTypes::Address> addressChannelUrlDirectory(
            new system::RoutingTypes::ChannelAddress(
                        messagingSettings.getChannelUrlDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getChannelUrlDirectoryParticipantId(), addressChannelUrlDirectory);
        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        joynrMessage.setHeaderExpiryDate(now + std::chrono::milliseconds(100));
    }

    ~MessageRouterTest() {
        std::remove(settingsFileName.c_str());
        delete messagingStubFactory;
    }

    void SetUp(){
        joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    }
    void TearDown(){
    }
protected:
    std::string settingsFileName;
    Settings settings;
    MessagingSettings messagingSettings;
    MockMessagingStubFactory* messagingStubFactory;
    MessageQueue* messageQueue;
    MessageRouter* messageRouter;
    JoynrMessage joynrMessage;
    void routeMessageToAddress(
            const std::string& destinationParticipantId,
            std::shared_ptr<system::RoutingTypes::Address> address);
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

TEST_F(MessageRouterTest, doNotAddMessageToQueue){
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    // this message should be added because no destination header set
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<system::RoutingTypes::ChannelAddress>(testHttp);
    messageRouter->addNextHop(testHttp, httpAddress);
    // the message now has a known destination and should be directly routed
    joynrMessage.setHeaderTo(testHttp);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto mqttAddress = std::make_shared<system::RoutingTypes::MqttAddress>(testMqtt);
    messageRouter->addNextHop(testMqtt, mqttAddress);
    // the message now has a known destination and should be directly routed
    joynrMessage.setHeaderTo(testMqtt);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);
}

TEST_F(MessageRouterTest, resendMessageWhenDestinationAddressIsAdded){
    const std::string testHttp = "TEST_HTTP";
    const std::string testMqtt = "TEST_MQTT";
    // this message should be added because destination is unknown
    joynrMessage.setHeaderTo(testHttp);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto httpAddress = std::make_shared<system::RoutingTypes::ChannelAddress>(testHttp);
    messageRouter->addNextHop(testHttp, httpAddress);
    EXPECT_EQ(messageQueue->getQueueLength(), 0);

    joynrMessage.setHeaderTo(testMqtt);
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    auto mqttAddress = std::make_shared<system::RoutingTypes::MqttAddress>(testMqtt);
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

void MessageRouterTest::routeMessageToAddress(
        const std::string& destinationParticipantId,
        std::shared_ptr<system::RoutingTypes::Address> address) {
    messageRouter->addNextHop(destinationParticipantId, address);
    joynrMessage.setHeaderTo(destinationParticipantId);
    ON_CALL(*messagingStubFactory, create(_)).WillByDefault(Return(nullptr));
    messageRouter->route(joynrMessage);
}

TEST_F(MessageRouterTest, routeMessageToHttpAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToHttpAddress";
    const std::string destinationChannelId = "TEST_routeMessageToHttpAddress_channelId";
    auto address = std::make_shared<system::RoutingTypes::ChannelAddress>(destinationChannelId);
    EXPECT_CALL(*messagingStubFactory,
            create(AllOf(
                    A<const joynr::system::RoutingTypes::Address&>(),
                    WhenDynamicCastTo<const system::RoutingTypes::ChannelAddress&>(
                            Property(&system::RoutingTypes::ChannelAddress::getChannelId, Eq(destinationChannelId))
                            )
                    ))
            ).Times(1);
    routeMessageToAddress(destinationParticipantId, address);
}

TEST_F(MessageRouterTest, routeMessageToMqttAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";
    const std::string destinationChannelId = "TEST_routeMessageToMqttAddress_channelId";
    auto address = std::make_shared<system::RoutingTypes::MqttAddress>(destinationChannelId);
    EXPECT_CALL(*messagingStubFactory,
            create(AllOf(
                    A<const joynr::system::RoutingTypes::Address&>(),
                    WhenDynamicCastTo<const system::RoutingTypes::MqttAddress&>(
                            Property(&system::RoutingTypes::MqttAddress::getChannelId, Eq(destinationChannelId))
                            )
                    ))
            ).Times(1);
    routeMessageToAddress(destinationParticipantId, address);
}

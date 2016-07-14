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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Semaphore.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/MessageRouter.h"
#include "tests/utils/MockObjects.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MessageQueue.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/SingleThreadedIOService.h"

using ::testing::Return;
using ::testing::Pointee;
using namespace joynr;

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}

class MessageRouterTest : public ::testing::Test {
public:
    MessageRouterTest() :
        settings(),
        messagingSettings(settings),
        messageQueue(nullptr),
        messagingStubFactory(nullptr),
        messageRouter(nullptr),
        joynrMessage(),
        singleThreadedIOService()
    {
        auto messageQueue = std::make_unique<MessageQueue>();
        this->messageQueue = messageQueue.get();

        auto messagingStubFactory = std::make_unique<MockMessagingStubFactory>();
        this->messagingStubFactory = messagingStubFactory.get();

        messageRouter = std::make_unique<MessageRouter>(std::move(messagingStubFactory),
                                                        std::unique_ptr<IPlatformSecurityManager>(),
                                                        singleThreadedIOService.getIOService(),
                                                        6, std::move(messageQueue));
        // provision global capabilities directory
        auto addressCapabilitiesDirectory =
                std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                    messagingSettings.getCapabilitiesDirectoryUrl() + messagingSettings.getCapabilitiesDirectoryChannelId() + "/",
                    messagingSettings.getCapabilitiesDirectoryChannelId());
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);
        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        joynrMessage.setHeaderExpiryDate(now + std::chrono::milliseconds(100));
    }

    ~MessageRouterTest() {
        std::remove(settingsFileName.c_str());
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
    MessageQueue* messageQueue;
    MockMessagingStubFactory* messagingStubFactory;
    std::unique_ptr<MessageRouter> messageRouter;
    JoynrMessage joynrMessage;
    void routeMessageToAddress(
            const std::string& destinationParticipantId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    SingleThreadedIOService singleThreadedIOService;

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
    EXPECT_CALL(*mockMessagingStub, transmit(joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()));
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);

    // add destination address -> message should be routed
    auto mqttAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, testMqtt);
    messageRouter->addNextHop(testMqtt, mqttAddress);
    // the message now has a known destination and should be directly routed
    joynrMessage.setHeaderTo(testMqtt);
    EXPECT_CALL(*messagingStubFactory, create(addressWithChannelId("mqtt", testMqtt))).Times(1).WillOnce(Return(mockMessagingStub));
    EXPECT_CALL(*mockMessagingStub, transmit(joynrMessage, A<const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&>()));
    messageRouter->route(joynrMessage);
    EXPECT_EQ(messageQueue->getQueueLength(), 1);
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

void MessageRouterTest::routeMessageToAddress(
        const std::string& destinationParticipantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address) {
    joynr::Semaphore semaphore(0);
    messageRouter->addNextHop(destinationParticipantId, address);
    joynrMessage.setHeaderTo(destinationParticipantId);
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
    EXPECT_CALL(*messagingStubFactory,
            create(addressWithChannelId("http", destinationChannelId))
            ).Times(1);
    routeMessageToAddress(destinationParticipantId, address);
}

TEST_F(MessageRouterTest, routeMessageToMqttAddress) {
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";
    const std::string destinationChannelId = "TEST_routeMessageToMqttAddress_channelId";
    const std::string brokerUri = "brokerUri";
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri, destinationChannelId);
    EXPECT_CALL(*messagingStubFactory,
            create(addressWithChannelId("mqtt", destinationChannelId))
            ).Times(1);
    routeMessageToAddress(destinationParticipantId, address);
}

TEST_F(MessageRouterTest, restoreRoutingTable) {
    const std::string routingTablePersistenceFilename = "test-RoutingTable.persist";
    std::remove(routingTablePersistenceFilename.c_str());

    auto messagingStubFactory = std::make_shared<MockMessagingStubFactory>();
    auto messageRouter = std::make_unique<MessageRouter>(messagingStubFactory, std::unique_ptr<IPlatformSecurityManager>(), singleThreadedIOService.getIOService());
    std::string participantId = "myParticipantId";
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();

    // MUST be done BEFORE savePersistence to pass the persistence filename to the publicationManager
    messageRouter->loadRoutingTable(routingTablePersistenceFilename);
    messageRouter->addProvisionedNextHop(participantId, address); // Saves the RoutingTable to the persistence file.

    messageRouter = std::make_unique<MessageRouter>(messagingStubFactory, std::unique_ptr<IPlatformSecurityManager>(), singleThreadedIOService.getIOService());

    messageRouter->loadRoutingTable(routingTablePersistenceFilename);

    joynrMessage.setHeaderTo(participantId);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*address))));
    messageRouter->route(joynrMessage);
}

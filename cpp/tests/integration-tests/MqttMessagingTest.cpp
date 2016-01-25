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
#include "AbstractMessagingTest.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "cluster-controller/messaging/joynr-messaging/MqttMessagingStubFactory.h"

using namespace ::testing;
using namespace joynr;

class MqttMessagingTest : public AbstractMessagingTest {
public:
    ADD_LOGGER(MqttMessagingTest);
    MqttMessagingTest()
    {
        // provision global capabilities directory
        std::shared_ptr<joynr::system::RoutingTypes::Address> addressCapabilitiesDirectory(
            new system::RoutingTypes::MqttAddress(
                        messagingSettings.getCapabilitiesDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);
        // provision channel url directory
        std::shared_ptr<joynr::system::RoutingTypes::Address> addressChannelUrlDirectory(
            new system::RoutingTypes::MqttAddress(
                        messagingSettings.getChannelUrlDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getChannelUrlDirectoryParticipantId(), addressChannelUrlDirectory);
        messagingStubFactory->registerStubFactory(new MqttMessagingStubFactory(mockMessageSender, senderChannelId));
    }

    void WaitXTimes(std::uint64_t x)
    {
        for(std::uint64_t i = 0; i<x; ++i) {
            ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
        }
    }

    ~MqttMessagingTest(){
        std::remove(settingsFileName.c_str());
    }
private:
    DISALLOW_COPY_AND_ASSIGN(MqttMessagingTest);
};

INIT_LOGGER(MqttMessagingTest);

TEST_F(MqttMessagingTest, sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager)
{
    // Test Outline: send message from JoynrMessageSender to ICommunicationManager
    // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
    //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
    // - InProcessMessagingStub.transmit (IMessaging)
    // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
    // - MessageRouter.route
    // - MessageRunnable.run
    // - MqttMessagingStub.transmit (IMessaging)
    // - MessageSender.send
    std::shared_ptr<system::RoutingTypes::MqttAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::MqttAddress>(new system::RoutingTypes::MqttAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(joynrMessagingEndpointAddr);
}


TEST_F(MqttMessagingTest, routeMsgWithInvalidParticipantId)
{
    routeMsgWithInvalidParticipantId();
}

TEST_F(MqttMessagingTest, routeMsgToInProcessMessagingSkeleton)
{
    routeMsgToInProcessMessagingSkeleton();
}


TEST_F(MqttMessagingTest, routeMsgToMqtt)
{
    std::shared_ptr<system::RoutingTypes::MqttAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::MqttAddress>(new system::RoutingTypes::MqttAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    routeMsgToCommunicationManager(joynrMessagingEndpointAddr);
}


TEST_F(MqttMessagingTest, routeMultipleMessages)
{
    std::shared_ptr<system::RoutingTypes::MqttAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::MqttAddress>(new system::RoutingTypes::MqttAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    routeMultipleMessages(joynrMessagingEndpointAddr);
}



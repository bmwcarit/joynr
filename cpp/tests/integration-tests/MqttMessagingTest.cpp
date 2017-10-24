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
#include "AbstractMessagingTest.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/BrokerUrl.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStubFactory.h"

using namespace ::testing;
using namespace joynr;

class MqttMessagingTest : public AbstractMessagingTest {
public:
    ADD_LOGGER(MqttMessagingTest)
    MqttMessagingTest() :
        mqttTopic("receiverChannelId"),
        brokerUri()
    {
        std::string brokerHost = messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getHost();
        std::string brokerPort = std::to_string(messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getPort());
        brokerUri = "tcp://" + brokerHost + ":" + brokerPort;
        // provision global capabilities directory
        const bool isGloballyVisible = true;
        const auto addressCapabilitiesDirectory = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerUri,messagingSettings.getCapabilitiesDirectoryChannelId());
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory, isGloballyVisible);

        messagingStubFactory->registerStubFactory(std::make_shared<MqttMessagingStubFactory>(mockMessageSender));
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

    std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> createJoynrMessagingEndpointAddress();
protected:
    const std::string mqttTopic;
private:
    DISALLOW_COPY_AND_ASSIGN(MqttMessagingTest);
    std::string brokerUri;
};

TEST_F(MqttMessagingTest, sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager)
{
    // Test Outline: send message from JoynrMessageSender to ICommunicationManager
    // - MessageSender.sendRequest (IMessageSender)
    //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
    // - InProcessMessagingStub.transmit (IMessaging)
    // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
    // - MessageRouter.route
    // - MessageRunnable.run
    // - MqttMessagingStub.transmit (IMessaging)
    // - MessageSender.send
    auto joynrMessagingEndpointAddr = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    joynrMessagingEndpointAddr->setTopic(mqttTopic);

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

std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> MqttMessagingTest::createJoynrMessagingEndpointAddress() {
    auto joynrMessagingEndpointAddr = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    joynrMessagingEndpointAddr->setTopic(mqttTopic);
    joynrMessagingEndpointAddr->setBrokerUri(brokerUri);
    return joynrMessagingEndpointAddr;
}

TEST_F(MqttMessagingTest, routeMsgToMqtt)
{
    routeMsgToCommunicationManager(createJoynrMessagingEndpointAddress());
}


TEST_F(MqttMessagingTest, routeMultipleMessages)
{
    routeMultipleMessages(createJoynrMessagingEndpointAddress());
}

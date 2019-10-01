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
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/HttpMessagingStubFactory.h"

using namespace ::testing;
using namespace joynr;

class HttpMessagingTest : public AbstractMessagingTest
{
public:
    ADD_LOGGER(HttpMessagingTest)
    HttpMessagingTest() : receiverChannelId("receiverChannelId"), _isLocalMessage(true)
    {
        // provision global capabilities directory
        const bool isGloballyVisible = true;
        auto addressCapabilitiesDirectory =
                std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                        _messagingSettings.getCapabilitiesDirectoryUrl() +
                                _messagingSettings.getCapabilitiesDirectoryChannelId() + "/",
                        _messagingSettings.getCapabilitiesDirectoryChannelId());
        _messageRouter->addProvisionedNextHop(
                _messagingSettings.getCapabilitiesDirectoryParticipantId(),
                addressCapabilitiesDirectory,
                isGloballyVisible);
        _messagingStubFactory->registerStubFactory(
                std::make_shared<HttpMessagingStubFactory>(_mockMessageSender));
    }

    ~HttpMessagingTest()
    {
    }

protected:
    const std::string receiverChannelId;
    const bool _isLocalMessage;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpMessagingTest);
};

TEST_F(HttpMessagingTest,
       sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager)
{
    // Test Outline: send message from JoynrMessageSender to ICommunicationManager
    // - MessageSender.sendRequest (IMessageSender)
    //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
    // - InProcessMessagingStub.transmit (IMessaging)
    // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
    // - MessageRouter.route
    // - MessageRunnable.run
    // - HttpMessagingStub.transmit (IMessaging)
    // - MessageSender.send
    auto joynrMessagingEndpointAddr =
            std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(
            joynrMessagingEndpointAddr);
}

TEST_F(HttpMessagingTest, routeMsgWithInvalidParticipantId)
{
    routeMsgWithInvalidParticipantId();
}

TEST_F(HttpMessagingTest, routeMsgToInProcessMessagingSkeleton)
{
    routeMsgToInProcessMessagingSkeleton();
}

TEST_F(HttpMessagingTest, routeMsgToHttpCommunicationMgr)
{
    auto joynrMessagingEndpointAddr =
            std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    routeMsgToCommunicationManager(joynrMessagingEndpointAddr);
}

TEST_F(HttpMessagingTest, routeMultipleMessages)
{
    auto joynrMessagingEndpointAddr =
            std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    routeMultipleMessages(joynrMessagingEndpointAddr);
}

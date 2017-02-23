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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/Request.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "tests/utils/MockObjects.h"
#include "joynr/SingleThreadedIOService.h"

using namespace ::testing;
using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

class AbstractMessagingTest : public ::testing::Test {
public:
    std::string settingsFileName;
    Settings settings;
    MessagingSettings messagingSettings;
    std::string senderId;
    std::string globalClusterControllerAddress;
    std::string receiverId;
    Request request;
    std::string requestId;
    MessagingQos qos;
    std::shared_ptr<MockInProcessMessagingSkeleton> inProcessMessagingSkeleton;
    Semaphore semaphore;
    const bool isLocalMessage;

    JoynrMessageFactory messageFactory;
    std::shared_ptr<MockMessageReceiver> mockMessageReceiver;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    std::shared_ptr<MessagingStubFactory> messagingStubFactory;
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<CcMessageRouter> messageRouter;
    AbstractMessagingTest() :
        settingsFileName("MessagingTest.settings"),
        settings(settingsFileName),
        messagingSettings(settings),
        senderId("senderParticipantId"),
        globalClusterControllerAddress("senderChannelId"),
        receiverId("receiverParticipantId"),
        request(),
        requestId("requestId"),
        qos(),
        inProcessMessagingSkeleton(std::make_shared<MockInProcessMessagingSkeleton>()),
        semaphore(0),
        isLocalMessage(false),
        messageFactory(),
        mockMessageReceiver(new MockMessageReceiver()),
        mockMessageSender(new MockMessageSender()),
        messagingStubFactory(std::make_shared<MessagingStubFactory>()),
        singleThreadedIOService(),
        messageRouter(nullptr)
    {
        const std::string globalCCAddress("globalAddress");

        messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());
        messageRouter = std::make_unique<CcMessageRouter>(messagingStubFactory,
                                                          std::make_shared<MulticastMessagingSkeletonDirectory>(),
                                                          nullptr,
                                                          singleThreadedIOService.getIOService(),
                                                          nullptr,
                                                          globalCCAddress);
        qos.setTtl(10000);
    }

    void WaitXTimes(std::uint64_t x)
    {
        for(std::uint64_t i = 0; i<x; ++i) {
            ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
        }
    }

    ~AbstractMessagingTest(){
        std::remove(settingsFileName.c_str());
    }

    void sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(
            std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        // Test Outline: send message from JoynrMessageSender to ICommunicationManager
        // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
        //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
        // - InProcessMessagingStub.transmit (IMessaging)
        // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
        // - MessageRouter.route
        // - MessageRunnable.run
        // - *MessagingStub.transmit (IMessaging)
        // - MessageSender.send

        MockDispatcher mockDispatcher;
        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(_,_))
                .Times(0);

        // MessageSender should receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,_,_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        EXPECT_CALL(mockDispatcher, addReplyCaller(_,_,_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        JoynrMessageSender messageSender(messageRouter);
        std::shared_ptr<IReplyCaller> replyCaller;
        messageSender.registerDispatcher(&mockDispatcher);

        messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr);

        messageSender.sendRequest(senderId, receiverId, qos, request, replyCaller, isLocalMessage);

        WaitXTimes(2);
    }

    void routeMsgWithInvalidParticipantId()
    {
        std::string invalidReceiverId("invalidReceiverId");
        JoynrMessage message = messageFactory.createRequest(
                    senderId,
                    invalidReceiverId,
                    qos,
                    request,
                    isLocalMessage);


        messageRouter->route(message);
        SUCCEED();
    }

    void routeMsgToInProcessMessagingSkeleton()
    {
        JoynrMessage message = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request,
                    isLocalMessage);

        // We must set the reply address here. Otherwise the message router will
        // set it and the message which was created will differ from the message
        // which is passed to the messaging-skeleton.
        message.setHeaderReplyAddress(globalClusterControllerAddress);

        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        // MessageSender should not receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,_,_))
                .Times(0);

        EXPECT_CALL(*mockMessageReceiver, getGlobalClusterControllerAddress())
                .Times(0);

        auto messagingSkeletonEndpointAddr = std::make_shared<InProcessMessagingAddress>(inProcessMessagingSkeleton);

        messageRouter->addNextHop(receiverId, messagingSkeletonEndpointAddr);

        messageRouter->route(message);

        WaitXTimes(1);
    }

    void routeMsgToCommunicationManager(std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        JoynrMessage message = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request,
                    isLocalMessage);
        message.setHeaderReplyAddress(globalClusterControllerAddress);

        // InProcessMessagingSkeleton should not receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message),_))
                .Times(0);

        // *CommunicationManager should receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,Eq(message),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr);

        messageRouter->route(message);

        WaitXTimes(1);
    }

    void routeMultipleMessages(std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        JoynrMessage message = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request,
                    isLocalMessage);
        message.setHeaderReplyAddress(globalClusterControllerAddress);

        std::string receiverId2("receiverId2");
        JoynrMessage message2 = messageFactory.createRequest(
                    senderId,
                    receiverId2,
                    qos,
                    request,
                    isLocalMessage);
        message2.setHeaderReplyAddress(globalClusterControllerAddress);

        // MessageSender should receive message
        EXPECT_CALL(*mockMessageSender, sendMessage(_, Eq(message),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        // InProcessMessagingSkeleton should receive twice message2
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message2),_))
                .Times(2).WillRepeatedly(ReleaseSemaphore(&semaphore));

        EXPECT_CALL(*mockMessageReceiver, getGlobalClusterControllerAddress())
                .WillRepeatedly(ReturnRefOfCopy(globalClusterControllerAddress));

        auto messagingSkeletonEndpointAddr = std::make_shared<InProcessMessagingAddress>(inProcessMessagingSkeleton);

        messageRouter->addNextHop(receiverId2, messagingSkeletonEndpointAddr);
        messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr);

        messageRouter->route(message);
        messageRouter->route(message2);
        messageRouter->route(message2);

        WaitXTimes(3);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractMessagingTest);
};


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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/MessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/Request.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "tests/utils/MockObjects.h"

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
    std::string senderChannelId;
    std::string receiverId;
    std::string receiverChannelId;
    Request request;
    std::string requestId;
    MessagingQos qos;
    std::shared_ptr<MockInProcessMessagingSkeleton> inProcessMessagingSkeleton;
    Semaphore semaphore;

    JoynrMessageFactory messageFactory;
    std::shared_ptr<MockMessageReceiver> mockMessageReceiver;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    MessagingStubFactory* messagingStubFactory;
    std::shared_ptr<MessageRouter> messageRouter;
    AbstractMessagingTest() :
        settingsFileName("MessagingTest.settings"),
        settings(settingsFileName),
        messagingSettings(settings),
        senderId("senderParticipantId"),
        senderChannelId("senderChannelId"),
        receiverId("receiverParticipantId"),
        receiverChannelId("receiverChannelId"),
        request(),
        requestId("requestId"),
        qos(),
        inProcessMessagingSkeleton(new MockInProcessMessagingSkeleton()),
        semaphore(0),
        messageFactory(),
        mockMessageReceiver(new MockMessageReceiver()),
        mockMessageSender(new MockMessageSender()),
        messagingStubFactory(new MessagingStubFactory()),
        messageRouter(new MessageRouter(messagingStubFactory, nullptr))
    {
        messagingStubFactory->registerStubFactory(new InProcessMessagingStubFactory());

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

        messageSender.sendRequest(senderId, receiverId, qos, request, replyCaller);

        WaitXTimes(2);
    }

    void routeMsgWithInvalidParticipantId()
    {
        std::string invalidReceiverId("invalidReceiverId");
        JoynrMessage message = messageFactory.createRequest(
                    senderId,
                    invalidReceiverId,
                    qos,
                    request);


        messageRouter->route(message);
        SUCCEED();
    }

    void routeMsgToInProcessMessagingSkeleton()
    {
        JoynrMessage message = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request);

        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        // MessageSender should not receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,_,_))
                .Times(0);

        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
                .Times(0);
    //            .WillOnce(ReturnRefOfCopy(senderChannelId));
    //            .WillRepeatedly(ReturnRefOfCopy(senderChannelId));

        std::shared_ptr<InProcessMessagingAddress> messagingSkeletonEndpointAddr =
                std::shared_ptr<InProcessMessagingAddress>(new InProcessMessagingAddress(inProcessMessagingSkeleton));

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
                    request);
        message.setHeaderReplyChannelId(senderChannelId);

        // InProcessMessagingSkeleton should not receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message),_))
                .Times(0);

        // *CommunicationManager should receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(Eq(receiverChannelId),Eq(message),_))
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
                    request);
        message.setHeaderReplyChannelId(senderChannelId);

        std::string receiverId2("receiverId2");
        JoynrMessage message2 = messageFactory.createRequest(
                    senderId,
                    receiverId2,
                    qos,
                    request);
        message2.setHeaderReplyChannelId(senderChannelId);

        // InProcessMessagingSkeleton should receive the message2 and message3
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message2),_))
                .Times(2).WillRepeatedly(ReleaseSemaphore(&semaphore));

        // MessageSender should receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(Eq(receiverChannelId), Eq(message),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
    //            .WillOnce(ReturnRefOfCopy(senderChannelId));
                .WillRepeatedly(ReturnRefOfCopy(senderChannelId));

        std::shared_ptr<InProcessMessagingAddress> messagingSkeletonEndpointAddr(
                    new InProcessMessagingAddress(inProcessMessagingSkeleton)
        );

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


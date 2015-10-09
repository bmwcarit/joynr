/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include <QFile>
#include "common/in-process/InProcessMessagingSkeleton.h"
#include "joynr/MessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/JoynrMessage.h"
#include "joynr/Dispatcher.h"
#include <QString>
#include <string>
#include "joynr/JoynrMessageFactory.h"
#include "joynr/system/RoutingTypes_QtChannelAddress.h"
#include "joynr/Request.h"
#include "tests/utils/MockObjects.h"
#include "joynr/InProcessMessagingAddress.h"
#include "common/in-process/InProcessMessagingStub.h"
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "cluster-controller/http-communication-manager/HttpSender.h"
#include "cluster-controller/messaging/joynr-messaging/JoynrMessagingStubFactory.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/Future.h"

using namespace ::testing;
using namespace joynr;



class MessagingTest : public ::testing::Test {
public:
    QString settingsFileName;
    QSettings settings;
    MessagingSettings messagingSettings;
    joynr_logging::Logger* logger;
    std::string senderId;
    QString senderChannelId;
    std::string receiverId;
    QString receiverChannelId;
    Request request;
    QString requestId;
    MessagingQos qos;
    std::shared_ptr<MockInProcessMessagingSkeleton> inProcessMessagingSkeleton;

    JoynrMessageFactory messageFactory;
    std::shared_ptr<MockMessageReceiver> mockMessageReceiver;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    MessagingStubFactory* messagingStubFactory;
    std::shared_ptr<MessageRouter> messageRouter;
    MessagingTest() :
        settingsFileName("MessagingTest.settings"),
        settings(settingsFileName, QSettings::IniFormat),
        messagingSettings(settings),
        logger(joynr_logging::Logging::getInstance()->getLogger("TEST", "MessagingTest")),
        senderId("senderParticipantId"),
        senderChannelId("senderChannelId"),
        receiverId("receiverParticipantId"),
        receiverChannelId("receiverChannelId"),
        request(),
        requestId("requestId"),
        qos(),
        inProcessMessagingSkeleton(new MockInProcessMessagingSkeleton()),
        messageFactory(),
        mockMessageReceiver(new MockMessageReceiver()),
        mockMessageSender(new MockMessageSender()),
        messagingStubFactory(new MessagingStubFactory()),
        messageRouter(new MessageRouter(messagingStubFactory, NULL))
    {
        // provision global capabilities directory
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> addressCapabilitiesDirectory(
            new system::RoutingTypes::QtChannelAddress(messagingSettings.getCapabilitiesDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId().toStdString(), addressCapabilitiesDirectory);
        // provision channel url directory
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> addressChannelUrlDirectory(
            new system::RoutingTypes::QtChannelAddress(messagingSettings.getChannelUrlDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getChannelUrlDirectoryParticipantId().toStdString(), addressChannelUrlDirectory);
        messagingStubFactory->registerStubFactory(new JoynrMessagingStubFactory(mockMessageSender, senderChannelId));
        messagingStubFactory->registerStubFactory(new InProcessMessagingStubFactory());

        qos.setTtl(10000);
    }
    ~MessagingTest(){
        QFile::remove(settingsFileName);
    }
private:
    DISALLOW_COPY_AND_ASSIGN(MessagingTest);
};

TEST_F(MessagingTest, sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager)
{
    // Test Outline: send message from JoynrMessageSender to ICommunicationManager
    // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
    //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
    // - InProcessMessagingStub.transmit (IMessaging)
    // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
    // - MessageRouter.route
    // - MessageRunnable.run
    // - JoynrMessagingStub.transmit (IMessaging)
    // - MessageSender.send


    MockDispatcher mockDispatcher;
    // InProcessMessagingSkeleton should receive the message
    EXPECT_CALL(*inProcessMessagingSkeleton, transmit(_))
            .Times(0);

    // MessageSender should receive the message
    EXPECT_CALL(*mockMessageSender, sendMessage(_,_))
            .Times(1);

    EXPECT_CALL(mockDispatcher, addReplyCaller(_,_,_))
            .Times(1);

    JoynrMessageSender messageSender(messageRouter);
    std::shared_ptr<IReplyCaller> replyCaller;
    messageSender.registerDispatcher(&mockDispatcher);

    std::shared_ptr<system::RoutingTypes::QtChannelAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::QtChannelAddress>(new system::RoutingTypes::QtChannelAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr);

    messageSender.sendRequest(senderId, receiverId, qos, request, replyCaller);
}


TEST_F(MessagingTest, routeMsgWithInvalidParticipantId)
{
    std::string invalidReceiverId("invalidReceiverId");
    JoynrMessage message = messageFactory.createRequest(
                QString::fromStdString(senderId),
                QString::fromStdString(invalidReceiverId),
                qos,
                request);


    messageRouter->route(message);
    SUCCEED();
}

TEST_F(MessagingTest, routeMsgToInProcessMessagingSkeleton)
{
    JoynrMessage message = messageFactory.createRequest(
                QString::fromStdString(senderId),
                QString::fromStdString(receiverId),
                qos,
                request);

    // InProcessMessagingSkeleton should receive the message
    EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message)))
            .Times(1);

    // MessageSender should not receive the message
    EXPECT_CALL(*mockMessageSender, sendMessage(_,_))
            .Times(0);

    EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
            .Times(0);
//            .WillOnce(ReturnRefOfCopy(senderChannelId));
//            .WillRepeatedly(ReturnRefOfCopy(senderChannelId));

    std::shared_ptr<InProcessMessagingAddress> messagingSkeletonEndpointAddr =
            std::shared_ptr<InProcessMessagingAddress>(new InProcessMessagingAddress(inProcessMessagingSkeleton));

    messageRouter->addNextHop(receiverId, messagingSkeletonEndpointAddr);

    messageRouter->route(message);
}

TEST_F(MessagingTest, DISABLED_routeMsgToLipciMessagingSkeleton)
{
// NOTE: LipciMessaging doesn't exists (2012-05-08)
//    std::shared_ptr<MockLipceMessagingSkeleton> messagingSkeleton(
//                new MockLipciMessagingSkeleton());

    JoynrMessage message = messageFactory.createRequest(
                QString::fromStdString(senderId),
                QString::fromStdString(receiverId),
                qos,
                request);

    // LipciMessagingSkeleton should receive the message
// NOTE: LipciMessaging doesn't exists (2012-05-08)
//    EXPECT_CALL(*messagingSkeleton, transmit(Eq(message),Eq(qos)))
//            .Times(1);

    // InProcessMessagingSkeleton should not receive the message
    EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message)))
            .Times(0);

    // MessageSender should not receive the message
    EXPECT_CALL(*mockMessageSender, sendMessage(_,_))
            .Times(0);

// NOTE: LipciMessaging doesn't exists (2012-05-08)
//    std::shared_ptr<LipciEndpointAddress> messagingSkeletonEndpointAddr =
//            std::shared_ptr<LipciEndpointAddress>(new LipciEndpointAddress(messagingSkeleton));

//    messageRouter->add(receiverId, messagingSkeletonEndpointAddr);
    messageRouter->route(message);
}


TEST_F(MessagingTest, routeMsgToHttpCommunicationMgr)
{
    JoynrMessage message = messageFactory.createRequest(
                QString::fromStdString(senderId),
                QString::fromStdString(receiverId),
                qos,
                request);
    message.setHeaderReplyChannelId(senderChannelId);

    // InProcessMessagingSkeleton should not receive the message
    EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message)))
            .Times(0);

    // HttpCommunicationManager should receive the message
    EXPECT_CALL(*mockMessageSender, sendMessage(Eq(receiverChannelId),Eq(message)))
            .Times(1);

    std::shared_ptr<system::RoutingTypes::QtChannelAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::QtChannelAddress>(new system::RoutingTypes::QtChannelAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr);

    messageRouter->route(message);
}


TEST_F(MessagingTest, routeMultipleMessages)
{
    JoynrMessage message = messageFactory.createRequest(
                QString::fromStdString(senderId),
                QString::fromStdString(receiverId),
                qos,
                request);
    message.setHeaderReplyChannelId(senderChannelId);

    std::string receiverId2("receiverId2");
    JoynrMessage message2 = messageFactory.createRequest(
                QString::fromStdString(senderId),
                QString::fromStdString(receiverId2),
                qos,
                request);
    message2.setHeaderReplyChannelId(senderChannelId);

    // InProcessMessagingSkeleton should receive the message2 and message3
    EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(message2)))
            .Times(2);

    // MessageSender should receive the message
    EXPECT_CALL(*mockMessageSender, sendMessage(Eq(receiverChannelId), Eq(message)))
            .Times(1);

    EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
//            .WillOnce(ReturnRefOfCopy(senderChannelId));
            .WillRepeatedly(ReturnRefOfCopy(senderChannelId));

    std::shared_ptr<InProcessMessagingAddress> messagingSkeletonEndpointAddr(
                new InProcessMessagingAddress(inProcessMessagingSkeleton)
    );

    messageRouter->addNextHop(receiverId2, messagingSkeletonEndpointAddr);

    std::shared_ptr<system::RoutingTypes::QtChannelAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::QtChannelAddress>(new system::RoutingTypes::QtChannelAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr);

    messageRouter->route(message);
    messageRouter->route(message2);
    messageRouter->route(message2);
}

// global function used for calls to the MockChannelUrlSelectorProxy
void messagingTestPseudoGetChannelUrls(std::shared_ptr<Future<types::QtChannelUrlInformation> > future , QString channelId, int timeout) {
    types::QtChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
    urlInformation.setUrls(urls);
    future->onSuccess(urlInformation);
}



TEST_F(MessagingTest, DISABLED_messageSenderGetsAndUsesDifferentUrlsForOneChannel) {
//    const QString settingsFileName ("test-resources/ChannelUrlSelectorTest.settings");
//    MessagingSettings* settings = new MessagingSettings(new QSettings(settingsFileName, QSettings::IniFormat));
//    int messageSendRetryInterval = 1000;

//    BounceProxyUrl bounceProxyUrl(settings->getBounceProxyUrl());
//    MessageSender* messageSender = new MessageSender(bounceProxyUrl, messageSendRetryInterval);
//    MockLocalChannelUrlDirectory mockDirectory;
//    messageSender->init(mockDirectory, *settings);

//    EXPECT_CALL(*mockDirectory, getUrlsForChannel(A<std::shared_ptr<Future<types::QtChannelUrlInformation> > >(), A<QString>(),A<int>()))
//            .WillOnce(Invoke(messagingTestPseudoGetChannelUrls));


//    infrastructure::IGlobalCapabilitiesDirectoryBase::INTERFACE_NAME;
//    // try to reach capabilities directory - capabilities request

//    JoynrMessage message = messageFactory.createRequest(
//                        senderId,
//                        receiverId,
//                        request,
//                        qos,
//                        requestId);

//    message.setHeader<QString>(JoynrMessage::HEADER_NAME_REPLY_TO, senderChannelId);
//    //TODO unfinished test!
//    //messageSender->sendMessage("channelId",11111, message);



//    delete settings;
}

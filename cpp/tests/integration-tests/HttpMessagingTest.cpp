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
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "cluster-controller/messaging/joynr-messaging/HttpMessagingStubFactory.h"

using namespace ::testing;
using namespace joynr;

class HttpMessagingTest : public AbstractMessagingTest {
public:
    ADD_LOGGER(HttpMessagingTest);
    HttpMessagingTest()
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
        messagingStubFactory->registerStubFactory(new HttpMessagingStubFactory(mockMessageSender, senderChannelId));
    }

    ~HttpMessagingTest(){
    }
private:
    DISALLOW_COPY_AND_ASSIGN(HttpMessagingTest);
};

INIT_LOGGER(HttpMessagingTest);

TEST_F(HttpMessagingTest, sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager)
{
    // Test Outline: send message from JoynrMessageSender to ICommunicationManager
    // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
    //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
    // - InProcessMessagingStub.transmit (IMessaging)
    // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
    // - MessageRouter.route
    // - MessageRunnable.run
    // - HttpMessagingStub.transmit (IMessaging)
    // - MessageSender.send
    std::shared_ptr<system::RoutingTypes::ChannelAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::ChannelAddress>(new system::RoutingTypes::ChannelAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(joynrMessagingEndpointAddr);
}


TEST_F(HttpMessagingTest, routeMsgWithInvalidParticipantId)
{
    routeMsgWithInvalidParticipantId();
}

TEST_F(HttpMessagingTest, routeMsgToInProcessMessagingSkeleton)
{
    routeMsgToInProcessMessagingSkeleton();
}

TEST_F(HttpMessagingTest, DISABLED_routeMsgToLipciMessagingSkeleton)
{
// NOTE: LipciMessaging doesn't exists (2012-05-08)
//    std::shared_ptr<MockLipceMessagingSkeleton> messagingSkeleton(
//                new MockLipciMessagingSkeleton());

    JoynrMessage message = messageFactory.createRequest(
                senderId,
                receiverId,
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


TEST_F(HttpMessagingTest, routeMsgToHttpCommunicationMgr)
{
    std::shared_ptr<system::RoutingTypes::ChannelAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::ChannelAddress>(new system::RoutingTypes::ChannelAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    routeMsgToCommunicationManager(joynrMessagingEndpointAddr);
}


TEST_F(HttpMessagingTest, routeMultipleMessages)
{
    std::shared_ptr<system::RoutingTypes::ChannelAddress> joynrMessagingEndpointAddr =
            std::shared_ptr<system::RoutingTypes::ChannelAddress>(new system::RoutingTypes::ChannelAddress());
    joynrMessagingEndpointAddr->setChannelId(receiverChannelId);

    routeMultipleMessages(joynrMessagingEndpointAddr);
}

// global function used for calls to the MockChannelUrlSelectorProxy
void messagingTestPseudoGetChannelUrls(std::shared_ptr<Future<types::ChannelUrlInformation> > future , std::string channelId, int timeout) {
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {"firstUrl", "secondUrl", "thirdUrl"};
    urlInformation.setUrls(urls);
    future->onSuccess(urlInformation);
}

TEST_F(HttpMessagingTest, DISABLED_messageSenderGetsAndUsesDifferentUrlsForOneChannel) {
//    const QString settingsFileName ("test-resources/ChannelUrlSelectorTest.settings");
//    MessagingSettings* settings = new MessagingSettings(new QSettings(settingsFileName, QSettings::IniFormat));
//    int messageSendRetryInterval = 1000;

//    BrokerUrl brokerUrl(settings->getBrokerUrl());
//    MessageSender* messageSender = new MessageSender(brokerUrl, messageSendRetryInterval);
//    MockLocalChannelUrlDirectory mockDirectory;
//    messageSender->init(mockDirectory, *settings);

//    EXPECT_CALL(*mockDirectory, getUrlsForChannel(A<std::shared_ptr<Future<types::ChannelUrlInformation> > >(), A<QString>(),A<int>()))
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

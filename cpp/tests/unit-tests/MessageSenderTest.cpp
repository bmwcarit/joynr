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
#include <string>
#include <vector>

#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/MessageSender.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "tests/utils/MockObjects.h"
#include "joynr/SingleThreadedIOService.h"

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using namespace joynr;

class MessageSenderTest : public ::testing::Test {
public:
    MessageSenderTest() :
        messageFactory(),
        postFix(),
        senderID(),
        receiverID(),
        requestID(),
        qosSettings(),
        mockDispatcher(),
        mockMessagingStub(),
        callBack(),
        singleThreadedIOService(),
        mockMessageRouter(std::make_shared<MockMessageRouter>(singleThreadedIOService.getIOService())),
        isLocalMessage(true)
    {
        singleThreadedIOService.start();
    }


    void SetUp(){
        postFix = "_" + util::createUuid();
        senderID = "senderId" + postFix;
        receiverID = "receiverID" + postFix;
        requestID = "requestId" + postFix;
        qosSettings = MessagingQos(456000);
    }

protected:
    MutableMessageFactory messageFactory;
    std::string postFix;
    std::string senderID;
    std::string receiverID;
    std::string requestID;
    MessagingQos qosSettings;
    MockDispatcher mockDispatcher;
    MockMessagingStub mockMessagingStub;
    std::shared_ptr<IReplyCaller> callBack;
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    const bool isLocalMessage;
};

typedef MessageSenderTest MessageSenderDeathTest;


TEST_F(MessageSenderTest, sendRequest_normal){
    Request request;
    request.setMethodName("methodName");
    request.setParams(42, std::string("value"));
    std::vector<std::string> paramDatatypes;
    paramDatatypes.push_back("java.lang.Integer");
    paramDatatypes.push_back("java.lang.String");
    request.setParamDatatypes(paramDatatypes);

    JoynrMessage message = messageFactory.createRequest(
                senderID,
                receiverID,
                qosSettings,
                request,
                isLocalMessage
    );

    EXPECT_CALL( *(mockMessageRouter.get()), route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST)),
                                                  Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_));

    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);
    messageSender.sendRequest(senderID, receiverID, qosSettings, request, callBack, isLocalMessage);
}

TEST_F(MessageSenderTest, sendOneWayRequest_normal){
    OneWayRequest oneWayRequest;
    oneWayRequest.setMethodName("methodName");
    oneWayRequest.setParams(42, std::string("value"));
    std::vector<std::string> paramDatatypes;
    paramDatatypes.push_back("java.lang.Integer");
    paramDatatypes.push_back("java.lang.String");
    oneWayRequest.setParamDatatypes(paramDatatypes);

    JoynrMessage message = messageFactory.createOneWayRequest(
                senderID,
                receiverID,
                qosSettings,
                oneWayRequest,
                isLocalMessage
    );

    EXPECT_CALL( *(mockMessageRouter.get()), route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY)),
                                                  Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_));

    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);
    messageSender.sendOneWayRequest(senderID, receiverID, qosSettings, oneWayRequest, isLocalMessage);
}


TEST_F(MessageSenderDeathTest, DISABLED_sendRequest_nullPayloadFails_death){
    EXPECT_CALL(*(mockMessageRouter.get()), route(_,_)).Times(0);

    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);

    Request jsonRequest;
    ASSERT_DEATH(messageSender.sendRequest(senderID, receiverID, qosSettings, jsonRequest, callBack, isLocalMessage), "Assertion.*");
}


TEST_F(MessageSenderTest, sendReply_normal){
    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);
    Reply reply;
    reply.setRequestReplyId(util::createUuid());
    reply.setResponse(std::string("response"));

    JoynrMessage message = messageFactory.createReply(
                senderID,
                receiverID,
                qosSettings,
                reply);

    EXPECT_CALL(*(mockMessageRouter.get()), route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_REPLY)),
                                                  Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_));

    messageSender.sendReply(senderID, receiverID, qosSettings, reply);
}

TEST_F(MessageSenderTest, sendSubscriptionRequest_normal){
    std::int64_t period = 2000;
    std::int64_t validity = 100000;
    std::int64_t publicationTtl = 1000;
    std::int64_t alert = 4000;
    auto qos = std::make_shared<PeriodicSubscriptionQos>(validity, publicationTtl, period, alert);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(qos);

    JoynrMessage message = messageFactory.createSubscriptionRequest(
                senderID,
                receiverID,
                qosSettings,
                subscriptionRequest,
                isLocalMessage);

    EXPECT_CALL(*mockMessageRouter, route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST)),
                                                  Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_));

    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);

    messageSender.sendSubscriptionRequest(senderID, receiverID, qosSettings, subscriptionRequest, isLocalMessage);
}

TEST_F(MessageSenderTest, sendBroadcastSubscriptionRequest_normal){
    std::int64_t minInterval = 2000;
    std::int64_t validity = 100000;
    std::int64_t publicationTtl = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(validity, publicationTtl, minInterval);

    BroadcastSubscriptionRequest subscriptionRequest;
    BroadcastFilterParameters filter;
    filter.setFilterParameter("MyParameter", "MyValue");
    subscriptionRequest.setFilterParameters(filter);
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("broadcastName");
    subscriptionRequest.setQos(qos);

    JoynrMessage message = messageFactory.createBroadcastSubscriptionRequest(
                senderID,
                receiverID,
                qosSettings,
                subscriptionRequest,
                isLocalMessage);

    EXPECT_CALL(*mockMessageRouter, route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)),
                                                  Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_));

    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);

    messageSender.sendBroadcastSubscriptionRequest(senderID, receiverID, qosSettings, subscriptionRequest, isLocalMessage);
}

//TODO implement sending a reply to a subscription request!
TEST_F(MessageSenderTest, DISABLED_sendSubscriptionReply_normal){
    std::string payload("subscriptionReply");
    EXPECT_CALL(*(mockMessageRouter.get()), route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY)),
                                                  Property(&JoynrMessage::getPayload, Eq(payload))),_));

    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);

//    messageSender.sendSubscriptionReply(util::createUuid(), payload, senderID, receiverID, qosSettings);
}

TEST_F(MessageSenderTest, sendPublication_normal){
    MessageSender messageSender(mockMessageRouter);
    messageSender.registerDispatcher(&mockDispatcher);
    SubscriptionPublication publication;
    publication.setSubscriptionId("ignoresubscriptionid");
    publication.setResponse(std::string("publication"));
    JoynrMessage message = messageFactory.createSubscriptionPublication(
                senderID,
                receiverID,
                qosSettings,
                publication);

    EXPECT_CALL(*(mockMessageRouter.get()), route(AllOf(Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION)),
                                                      Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_));

    messageSender.sendSubscriptionPublication(senderID, receiverID, qosSettings, std::move(publication));
}

TEST_F(MessageSenderTest, sendMulticastSubscriptionRequest) {
    const std::string senderParticipantId("senderParticipantId");
    const std::string receiverParticipantId("receiverParticipantId");
    MessagingQos messagingQos(1, MessagingQosEffort::Enum::BEST_EFFORT);
    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();

    MulticastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName("subscribeToName");
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setQos(subscriptionQos);
    subscriptionRequest.setMulticastId("multicastId");

    JoynrMessage expectedMessage = messageFactory.createMulticastSubscriptionRequest(
            senderParticipantId,
            receiverParticipantId,
            messagingQos,
            subscriptionRequest,
            isLocalMessage);

    EXPECT_CALL(*mockMessageRouter, route(AllOf(
            Property(&JoynrMessage::getType, Eq(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)),
            Property(&JoynrMessage::getPayload, Eq(expectedMessage.getPayload()))),_));

    MessageSender messageSender(mockMessageRouter);

    messageSender.sendMulticastSubscriptionRequest(
            senderParticipantId,
            receiverParticipantId,
            messagingQos,
            subscriptionRequest,
            isLocalMessage);
}

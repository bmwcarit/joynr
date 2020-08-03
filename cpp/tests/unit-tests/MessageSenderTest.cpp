/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/MessagingQos.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/MessageSender.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/BroadcastFilterParameters.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/SingleThreadedIOService.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockMessagingStub.h"
#include "tests/mock/MockMessageRouter.h"

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using namespace joynr;

class MessageSenderTest : public ::testing::Test
{
public:
    MessageSenderTest()
            : messageFactory(),
              postFix(),
              senderID(),
              receiverID(),
              requestID(),
              qosSettings(),
              mockDispatcher(std::make_shared<MockDispatcher>()),
              mockMessagingStub(),
              callBack(),
              singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              mockMessageRouter(
                      std::make_shared<MockMessageRouter>(singleThreadedIOService->getIOService())),
              isLocalMessage(true)
    {
        singleThreadedIOService->start();
    }

    ~MessageSenderTest()
    {
        singleThreadedIOService->stop();
    }

    void SetUp()
    {
        postFix = "_" + util::createUuid();
        senderID = "senderId" + postFix;
        receiverID = "receiverID" + postFix;
        requestID = "requestId" + postFix;
        qosSettings = MessagingQos(456000);
    }

    void expectRoutedMessage(const std::string& type, const std::string& payload)
    {
        EXPECT_CALL(*(mockMessageRouter.get()),
                    route(AllOf(MessageHasType(type), ImmutableMessageHasPayload(payload)), _));
    }

protected:
    MutableMessageFactory messageFactory;
    std::string postFix;
    std::string senderID;
    std::string receiverID;
    std::string requestID;
    MessagingQos qosSettings;
    std::shared_ptr<MockDispatcher> mockDispatcher;
    MockMessagingStub mockMessagingStub;
    std::shared_ptr<IReplyCaller> callBack;
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    const bool isLocalMessage;
};

typedef MessageSenderTest MessageSenderDeathTest;

TEST_F(MessageSenderTest, sendRequest_normal)
{
    Request request;
    request.setMethodName("methodName");
    request.setParams(42, std::string("value"));
    std::vector<std::string> paramDatatypes;
    paramDatatypes.push_back("java.lang.Integer");
    paramDatatypes.push_back("java.lang.String");
    request.setParamDatatypes(paramDatatypes);

    MutableMessage mutableMessage = messageFactory.createRequest(
            senderID, receiverID, qosSettings, request, isLocalMessage);

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_REQUEST(), mutableMessage.getPayload());

    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);
    messageSender.sendRequest(senderID, receiverID, qosSettings, request, callBack, isLocalMessage);
}

TEST_F(MessageSenderTest, sendOneWayRequest_normal)
{
    OneWayRequest oneWayRequest;
    oneWayRequest.setMethodName("methodName");
    oneWayRequest.setParams(42, std::string("value"));
    std::vector<std::string> paramDatatypes;
    paramDatatypes.push_back("java.lang.Integer");
    paramDatatypes.push_back("java.lang.String");
    oneWayRequest.setParamDatatypes(paramDatatypes);

    MutableMessage mutableMessage = messageFactory.createOneWayRequest(
            senderID, receiverID, qosSettings, oneWayRequest, isLocalMessage);

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_ONE_WAY(), mutableMessage.getPayload());

    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);
    messageSender.sendOneWayRequest(
            senderID, receiverID, qosSettings, oneWayRequest, isLocalMessage);
}

TEST_F(MessageSenderTest, sendReply_normal)
{
    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);
    Reply reply;
    reply.setRequestReplyId(util::createUuid());
    reply.setResponse(std::string("response"));

    MutableMessage mutableMessage =
            messageFactory.createReply(senderID, receiverID, qosSettings, {}, reply);

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_REPLY(), mutableMessage.getPayload());

    messageSender.sendReply(senderID, receiverID, qosSettings, {}, reply);
}

TEST_F(MessageSenderTest, sendSubscriptionRequest_normal)
{
    std::int64_t period = 2000;
    std::int64_t validity = 100000;
    std::int64_t publicationTtl = 1000;
    std::int64_t alert = 4000;
    auto qos = std::make_shared<PeriodicSubscriptionQos>(validity, publicationTtl, period, alert);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(qos);

    MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(
            senderID, receiverID, qosSettings, subscriptionRequest, isLocalMessage);

    expectRoutedMessage(
            Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST(), mutableMessage.getPayload());

    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);

    messageSender.sendSubscriptionRequest(
            senderID, receiverID, qosSettings, subscriptionRequest, isLocalMessage);
}

TEST_F(MessageSenderTest, sendBroadcastSubscriptionRequest_normal)
{
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

    MutableMessage mutableMessage = messageFactory.createBroadcastSubscriptionRequest(
            senderID, receiverID, qosSettings, subscriptionRequest, isLocalMessage);

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST(),
                        mutableMessage.getPayload());

    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);

    messageSender.sendBroadcastSubscriptionRequest(
            senderID, receiverID, qosSettings, subscriptionRequest, isLocalMessage);
}

// TODO implement sending a reply to a subscription request!
TEST_F(MessageSenderTest, DISABLED_sendSubscriptionReply_normal)
{
    std::string payload("subscriptionReply");

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY(), payload);

    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);

    //    messageSender.sendSubscriptionReply(util::createUuid(), payload, senderID, receiverID,
    //    qosSettings);
}

TEST_F(MessageSenderTest, sendPublication_normal)
{
    MessageSender messageSender(mockMessageRouter, nullptr);
    messageSender.registerDispatcher(mockDispatcher);
    SubscriptionPublication publication;
    publication.setSubscriptionId("ignoresubscriptionid");
    publication.setResponse(std::string("publication"));

    MutableMessage mutableMessage = messageFactory.createSubscriptionPublication(
            senderID, receiverID, qosSettings, publication);

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_PUBLICATION(), mutableMessage.getPayload());

    messageSender.sendSubscriptionPublication(
            senderID, receiverID, qosSettings, std::move(publication));
}

TEST_F(MessageSenderTest, sendMulticastSubscriptionRequest)
{
    const std::string senderParticipantId("senderParticipantId");
    const std::string receiverParticipantId("receiverParticipantId");
    const std::string subscriptionId("subscriptionId");
    MessagingQos messagingQos(1, MessagingQosEffort::Enum::BEST_EFFORT);
    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();

    MulticastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName("subscribeToName");
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setQos(subscriptionQos);
    subscriptionRequest.setMulticastId("multicastId");

    MutableMessage mutableMessage =
            messageFactory.createMulticastSubscriptionRequest(senderParticipantId,
                                                              receiverParticipantId,
                                                              messagingQos,
                                                              subscriptionRequest,
                                                              isLocalMessage);

    SubscriptionReply subscriptionReply;
    subscriptionReply.setSubscriptionId("subscriptionId");

    MutableMessage mutableReplyMessage =
            messageFactory.createSubscriptionReply(receiverParticipantId,
                    senderParticipantId,
                    messagingQos,
                    subscriptionReply);

    expectRoutedMessage(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY(),
                        mutableReplyMessage.getPayload());

    MessageSender messageSender(mockMessageRouter, nullptr);

    messageSender.sendMulticastSubscriptionRequest(senderParticipantId,
                                                   receiverParticipantId,
                                                   messagingQos,
                                                   subscriptionRequest,
                                                   isLocalMessage);
}

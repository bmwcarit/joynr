/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#include <chrono>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "joynr/Message.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/TimePoint.h"

using namespace joynr;

class MutableMessageFactoryTest : public ::testing::Test
{
public:
    MutableMessageFactoryTest()
            : messageFactory(0),
              senderID(),
              receiverID(),
              requestReplyID(),
              isLocalMessage(true),
              qos(),
              request(),
              oneWayRequest(),
              reply(),
              subscriptionPublication()
    {
    }

    void SetUp()
    {
        senderID = std::string("senderId");
        receiverID = std::string("receiverID");
        requestReplyID = "requestReplyID";
        qos = MessagingQos(456000);
        qos.putCustomMessageHeader("test-header", "test-header-value");
        request.setMethodName("methodName");
        request.setRequestReplyId(requestReplyID);
        request.setParams(42, "value");
        request.setParamDatatypes({"java.lang.Integer", "java.lang.String"});
        oneWayRequest.setMethodName("methodName");
        oneWayRequest.setParams(42, "value");
        oneWayRequest.setParamDatatypes({"java.lang.Integer", "java.lang.String"});
        reply.setRequestReplyId(requestReplyID);
        reply.setResponse("response");

        std::string subscriptionId("subscriptionTestId");
        subscriptionPublication.setSubscriptionId(subscriptionId);
        subscriptionPublication.setResponse("publication");
    }

    void checkSenderRecipient(const MutableMessage& mutableMessage)
    {
        EXPECT_EQ(senderID, mutableMessage.getSender());
        EXPECT_EQ(receiverID, mutableMessage.getRecipient());
        boost::optional<std::string> customHeader = mutableMessage.getCustomHeader("test-header");
        ASSERT_TRUE(customHeader.is_initialized());
        EXPECT_EQ("test-header-value", *customHeader);
    }

    void checkRequest(const MutableMessage& mutableMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.Request",)";
        expectedPayloadStream << R"("methodName":"methodName",)";
        expectedPayloadStream << R"("paramDatatypes":["java.lang.Integer","java.lang.String"],)";
        expectedPayloadStream << R"("params":[42,"value"],)";
        expectedPayloadStream << R"("requestReplyId":")" << request.getRequestReplyId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, mutableMessage.getPayload());
    }

    void checkOneWayRequest(const MutableMessage& mutableMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.OneWayRequest",)";
        expectedPayloadStream << R"("methodName":"methodName",)";
        expectedPayloadStream << R"("paramDatatypes":["java.lang.Integer","java.lang.String"],)";
        expectedPayloadStream << R"("params":[42,"value"])";
        expectedPayloadStream << R"(})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, mutableMessage.getPayload());
    }

    void checkReply(const MutableMessage& mutableMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.Reply",)";
        expectedPayloadStream << R"("response":["response"],)";
        expectedPayloadStream << R"("requestReplyId":")" << reply.getRequestReplyId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, mutableMessage.getPayload());
    }

    void checkSubscriptionPublication(const MutableMessage& mutableMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.SubscriptionPublication",)";
        expectedPayloadStream << R"("response":["publication"],)";
        expectedPayloadStream << R"("subscriptionId":")"
                              << subscriptionPublication.getSubscriptionId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, mutableMessage.getPayload());
    }

    void checkMulticastPublication(const MutableMessage& mutableMessage,
                                   const MulticastPublication& multicastPublication)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.MulticastPublication",)";
        expectedPayloadStream << R"("response":["publication"],)";
        expectedPayloadStream << R"("multicastId":")" << multicastPublication.getMulticastId()
                              << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, mutableMessage.getPayload());
    }

protected:
    ADD_LOGGER(MutableMessageFactoryTest)
    MutableMessageFactory messageFactory;
    std::string senderID;
    std::string receiverID;
    std::string requestReplyID;
    const bool isLocalMessage;
    MessagingQos qos;
    Request request;
    OneWayRequest oneWayRequest;
    Reply reply;
    SubscriptionPublication subscriptionPublication;
};

TEST_F(MutableMessageFactoryTest, createRequest_withContentType)
{
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    checkSenderRecipient(mutableMessage);
}

TEST_F(MutableMessageFactoryTest, createRequest)
{
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    const TimePoint expectedExpiryDate = TimePoint::fromRelativeMs(static_cast<std::int64_t>(qos.getTtl()));
    const TimePoint expiryDate = mutableMessage.getExpiryDate();
    EXPECT_NEAR(expectedExpiryDate.toMilliseconds(), expiryDate.toMilliseconds(), 100.);
    JOYNR_LOG_DEBUG(
            logger(), "expiryDate: {} [{}]", expiryDate.toString(), expiryDate.toMilliseconds());
    JOYNR_LOG_DEBUG(logger(),
                    "expectedExpiryDate: {}  [{}]",
                    expectedExpiryDate.toString(),
                    expectedExpiryDate.toMilliseconds());

    checkSenderRecipient(mutableMessage);
    checkRequest(mutableMessage);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_REQUEST(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createReply)
{
    MutableMessage mutableMessage =
            messageFactory.createReply(senderID, receiverID, qos, {}, reply);
    checkSenderRecipient(mutableMessage);
    checkReply(mutableMessage);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_REPLY(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createOneWayRequest)
{
    MutableMessage mutableMessage = messageFactory.createOneWayRequest(
            senderID, receiverID, qos, oneWayRequest, isLocalMessage);
    checkSenderRecipient(mutableMessage);
    checkOneWayRequest(mutableMessage);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_ONE_WAY(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createPublication)
{
    MutableMessage mutableMessage = messageFactory.createSubscriptionPublication(
            senderID, receiverID, qos, subscriptionPublication);
    checkSenderRecipient(mutableMessage);
    checkSubscriptionPublication(mutableMessage);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_PUBLICATION(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createMulticastPublication)
{
    MulticastPublication multicastPublication;
    multicastPublication.setMulticastId(receiverID);
    multicastPublication.setResponse("publication");
    MutableMessage mutableMessage =
            messageFactory.createMulticastPublication(senderID, qos, multicastPublication);
    checkSenderRecipient(mutableMessage);
    checkMulticastPublication(mutableMessage, multicastPublication);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_MULTICAST(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createSubscriptionRequest)
{
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>();
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(subscriptionQos);
    MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(
            senderID, receiverID, qos, subscriptionRequest, isLocalMessage);
    checkSenderRecipient(mutableMessage);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createMulticastSubscriptionRequest)
{
    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    MulticastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setMulticastId("multicastId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(subscriptionQos);
    MutableMessage mutableMessage = messageFactory.createMulticastSubscriptionRequest(
            senderID, receiverID, qos, subscriptionRequest, isLocalMessage);
    checkSenderRecipient(mutableMessage);
    EXPECT_EQ(
            Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, createSubscriptionStop)
{
    std::string subscriptionId("TEST-SubscriptionId");
    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionId);
    MutableMessage mutableMessage =
            messageFactory.createSubscriptionStop(senderID, receiverID, qos, subscriptionStop);
    checkSenderRecipient(mutableMessage);
    EXPECT_EQ(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP(), mutableMessage.getType());
}

TEST_F(MutableMessageFactoryTest, testSetNoEffortHeader)
{
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    const boost::optional<std::string> optionalEffort = mutableMessage.getEffort();
    EXPECT_FALSE(optionalEffort);
}

TEST_F(MutableMessageFactoryTest, testSetBestEffortHeader)
{
    qos.setEffort(MessagingQosEffort::Enum::BEST_EFFORT);
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    const boost::optional<std::string> optionalEffort = mutableMessage.getEffort();
    ASSERT_TRUE(optionalEffort.is_initialized());
    EXPECT_EQ(
            MessagingQosEffort::getLiteral(MessagingQosEffort::Enum::BEST_EFFORT), *optionalEffort);
}

TEST_F(MutableMessageFactoryTest, compressFlagIsPropagated)
{
    const bool compress = true;
    qos.setCompress(compress);
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    EXPECT_EQ(compress, mutableMessage.getCompress());
}

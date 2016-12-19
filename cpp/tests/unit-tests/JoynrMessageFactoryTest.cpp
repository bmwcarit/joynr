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
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

#include "joynr/JoynrMessageFactory.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/MessagingQos.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MulticastPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/OnChangeSubscriptionQos.h"

using namespace joynr;

class JoynrMessageFactoryTest : public ::testing::Test
{
public:
    JoynrMessageFactoryTest()
            : messageFactory(0),
              senderID(),
              receiverID(),
              requestReplyID(),
              qos(),
              request(),
              oneWayRequest(),
              reply(),
              subscriptionPublication(),
              ttl(1000),
              ttlUplift(10000),
              upliftedTtl(ttl + ttlUplift),
              messagingQos(),
              factoryWithTtlUplift(ttlUplift)
    {
                  messagingQos.setTtl(ttl);
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
        request.setParamDatatypes({"java.lang.Integer","java.lang.String"});
        oneWayRequest.setMethodName("methodName");
        oneWayRequest.setParams(42, "value");
        oneWayRequest.setParamDatatypes({"java.lang.Integer","java.lang.String"});
        reply.setRequestReplyId(requestReplyID);
        reply.setResponse("response");

        std::string subscriptionId("subscriptionTestId");
        subscriptionPublication.setSubscriptionId(subscriptionId);
        subscriptionPublication.setResponse("publication");
    }

    void checkHeaderCreatorFromTo(const JoynrMessage& joynrMessage)
    {
        EXPECT_TRUE(joynrMessage.containsHeaderCreatorUserId());
        EXPECT_STREQ(senderID.c_str(), joynrMessage.getHeaderFrom().c_str());
        EXPECT_STREQ(receiverID.c_str(), joynrMessage.getHeaderTo().c_str());
        EXPECT_TRUE(joynrMessage.containsCustomHeader("test-header"));
        EXPECT_STREQ("test-header-value", joynrMessage.getCustomHeader("test-header").c_str());
    }

    void checkRequest(const JoynrMessage& joynrMessage)
    {
        // TODO create expected string from params and methodName
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.Request",)";
        expectedPayloadStream << R"("methodName":"methodName",)";
        expectedPayloadStream << R"("paramDatatypes":["java.lang.Integer","java.lang.String"],)";
        expectedPayloadStream << R"("params":[42,"value"],)";
        expectedPayloadStream << R"("requestReplyId":")" << request.getRequestReplyId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkOneWayRequest(const JoynrMessage& joynrMessage)
    {
        // TODO create expected string from params and methodName
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.OneWayRequest",)";
        expectedPayloadStream << R"("methodName":"methodName",)";
        expectedPayloadStream << R"("paramDatatypes":["java.lang.Integer","java.lang.String"],)";
        expectedPayloadStream << R"("params":[42,"value"])";
        expectedPayloadStream << R"(})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkReply(const JoynrMessage& joynrMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.Reply",)";
        expectedPayloadStream << R"("response":["response"],)";
        expectedPayloadStream << R"("requestReplyId":")" << reply.getRequestReplyId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkSubscriptionPublication(const JoynrMessage& joynrMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.SubscriptionPublication",)";
        expectedPayloadStream << R"("response":["publication"],)";
        expectedPayloadStream << R"("subscriptionId":")"
                              << subscriptionPublication.getSubscriptionId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkMulticastPublication(const JoynrMessage& joynrMessage,
                                   const MulticastPublication& multicastPublication)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.MulticastPublication",)";
        expectedPayloadStream << R"("response":["publication"],)";
        expectedPayloadStream << R"("multicastId":")"
                              << multicastPublication.getMulticastId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkMessageExpiryDate(const JoynrMessage& message, const std::int64_t expectedTtl);

protected:
    ADD_LOGGER(JoynrMessageFactoryTest);
    JoynrMessageFactory messageFactory;
    std::string senderID;
    std::string receiverID;
    std::string requestReplyID;
    MessagingQos qos;
    Request request;
    OneWayRequest oneWayRequest;
    Reply reply;
    SubscriptionPublication subscriptionPublication;

    const std::int64_t ttl;
    const std::uint64_t ttlUplift;
    const std::int64_t upliftedTtl;
    MessagingQos messagingQos;
    JoynrMessageFactory factoryWithTtlUplift;
};

INIT_LOGGER(JoynrMessageFactoryTest);

TEST_F(JoynrMessageFactoryTest, createRequest_withContentType)
{
    JoynrMessage joynrMessage = messageFactory.createRequest(senderID, receiverID, qos, request);
    checkHeaderCreatorFromTo(joynrMessage);
}

TEST_F(JoynrMessageFactoryTest, createRequest)
{
    JoynrMessage joynrMessage = messageFactory.createRequest(senderID, receiverID, qos, request);
    // warning if prepareRequest needs to long this assert will fail as it compares absolute
    // timestamps
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    JoynrTimePoint expectedExpiryDate = now + std::chrono::milliseconds(qos.getTtl());
    JoynrTimePoint expiryDate = joynrMessage.getHeaderExpiryDate();
    EXPECT_NEAR(expectedExpiryDate.time_since_epoch().count(),
                expiryDate.time_since_epoch().count(),
                100.);
    JOYNR_LOG_DEBUG(logger,
                    "expiryDate: {} [{}]",
                    DispatcherUtils::convertAbsoluteTimeToTtlString(expiryDate),
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            expiryDate.time_since_epoch()).count());
    JOYNR_LOG_DEBUG(logger,
                    "expectedExpiryDate: {}  [{}]",
                    DispatcherUtils::convertAbsoluteTimeToTtlString(expectedExpiryDate),
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            expectedExpiryDate.time_since_epoch()).count());

    checkHeaderCreatorFromTo(joynrMessage);
    checkRequest(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createReply)
{
    JoynrMessage joynrMessage = messageFactory.createReply(senderID, receiverID, qos, reply);
    checkHeaderCreatorFromTo(joynrMessage);
    checkReply(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_REPLY, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createOneWayRequest)
{
    JoynrMessage joynrMessage =
            messageFactory.createOneWayRequest(senderID, receiverID, qos, oneWayRequest);
    checkHeaderCreatorFromTo(joynrMessage);
    checkOneWayRequest(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY, joynrMessage.getType());
}

// TEST_F(JoynrMessageFactoryTest, createSubscriptionReply){
//    std::string subscriptionId("subscriptionTestId");
//    JoynrMessage joynrMessage = JoynrMessageFactory::prepareSubscriptionReply(senderID,
//    receiverID, payload, subscriptionId);
//    checkHeaderCreatorFromTo(joynrMessage);
//    checkPayload(joynrMessage);
//    EXPECT_QSTREQ(subscriptionId,
//    joynrMessage.getHeader<std::string>(JoynrMessage::HEADER_NAME_SUBSCRIPTION_ID));
//    EXPECT_QSTREQ(JoynrMessage::MESSAGE_TYPE_SUBSCRIPTION_REPLY, joynrMessage.getType());
//}

TEST_F(JoynrMessageFactoryTest, createPublication)
{
    JoynrMessage joynrMessage = messageFactory.createSubscriptionPublication(
            senderID, receiverID, qos, subscriptionPublication);
    checkHeaderCreatorFromTo(joynrMessage);
    checkSubscriptionPublication(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createMulticastPublication)
{
    MulticastPublication multicastPublication;
    multicastPublication.setMulticastId(receiverID);
    multicastPublication.setResponse("publication");
    JoynrMessage joynrMessage = messageFactory.createMulticastPublication(
            senderID, qos, multicastPublication);
    checkHeaderCreatorFromTo(joynrMessage);
    checkMulticastPublication(joynrMessage, multicastPublication);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createSubscriptionRequest)
{
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>();
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(subscriptionQos);
    JoynrMessage joynrMessage = messageFactory.createSubscriptionRequest(
            senderID, receiverID, qos, subscriptionRequest);
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createMulticastSubscriptionRequest)
{
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>();
    MulticastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setMulticastId("multicastId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(subscriptionQos);
    JoynrMessage joynrMessage = messageFactory.createMulticastSubscriptionRequest(
            senderID, receiverID, qos, subscriptionRequest);
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createSubscriptionStop)
{
    std::string subscriptionId("TEST-SubscriptionId");
    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionId);
    JoynrMessage joynrMessage =
            messageFactory.createSubscriptionStop(senderID, receiverID, qos, subscriptionStop);
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, testRequestContentType)
{
    Request request;
    request.setMethodName("methodName");
    request.setParams(std::string("test"));

    JoynrMessage message = messageFactory.createRequest(senderID, receiverID, qos, request);
    EXPECT_EQ(JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON, message.getHeaderContentType());
}

TEST_F(JoynrMessageFactoryTest, testSetNoEffortHeader)
{
    JoynrMessage message = messageFactory.createRequest(senderID, receiverID, qos, request);
    EXPECT_FALSE(message.containsHeaderEffort());
}

TEST_F(JoynrMessageFactoryTest, testSetBestEffortHeader)
{
    qos.setEffort(MessagingQosEffort::Enum::BEST_EFFORT);
    JoynrMessage message = messageFactory.createRequest(senderID, receiverID, qos, request);
    EXPECT_TRUE(message.containsHeaderEffort());
    EXPECT_EQ(MessagingQosEffort::getLiteral(MessagingQosEffort::Enum::BEST_EFFORT),
              message.getHeaderEffort());
}

void JoynrMessageFactoryTest::checkMessageExpiryDate(const JoynrMessage& message, const std::int64_t expectedTtl) {
    const std::int64_t tolerance = 10;
    std::int64_t diff = expectedTtl
            - std::chrono::duration_cast<std::chrono::milliseconds>(
                message.getHeaderExpiryDate() - std::chrono::system_clock::now())
            .count();
    EXPECT_GE(diff, 0);
    EXPECT_LE(std::abs(diff), tolerance);
}

TEST_F(JoynrMessageFactoryTest, testDefaultTtlUplift)
{
    JoynrMessage message = messageFactory.createRequest(senderID, receiverID, messagingQos, request);

    checkMessageExpiryDate(message, ttl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_Request)
{
    JoynrMessage message = factoryWithTtlUplift.createRequest(senderID, receiverID, messagingQos, request);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_Reply_noUplift)
{
    JoynrMessage message = factoryWithTtlUplift.createReply(senderID, receiverID, messagingQos, reply);

    checkMessageExpiryDate(message, ttl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_OneWayRequest)
{
    JoynrMessage message = factoryWithTtlUplift.createOneWayRequest(senderID, receiverID, messagingQos, oneWayRequest);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_MulticastPublication)
{
    MulticastPublication publication;
    JoynrMessage message = factoryWithTtlUplift.createMulticastPublication(senderID, messagingQos, publication);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_SubscriptionPublication)
{
    JoynrMessage message = factoryWithTtlUplift.createSubscriptionPublication(senderID, receiverID, messagingQos, subscriptionPublication);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_SubscriptionRequest)
{
    SubscriptionRequest request;
    JoynrMessage message = factoryWithTtlUplift.createSubscriptionRequest(senderID, receiverID, messagingQos, request);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_MulticastSubscriptionRequest)
{
    MulticastSubscriptionRequest request;
    JoynrMessage message = factoryWithTtlUplift.createMulticastSubscriptionRequest(senderID, receiverID, messagingQos, request);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_BroadcastSubscriptionRequest)
{
    BroadcastSubscriptionRequest request;
    JoynrMessage message = factoryWithTtlUplift.createBroadcastSubscriptionRequest(senderID, receiverID, messagingQos, request);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_SubscriptionReply_noUplift)
{
    SubscriptionReply reply;
    JoynrMessage message = factoryWithTtlUplift.createSubscriptionReply(senderID, receiverID, messagingQos, reply);

    checkMessageExpiryDate(message, ttl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUplift_SubscriptionStop)
{
    SubscriptionStop subscriptionStop;
    JoynrMessage message = factoryWithTtlUplift.createSubscriptionStop(senderID, receiverID, messagingQos, subscriptionStop);

    checkMessageExpiryDate(message, upliftedTtl);
}

TEST_F(JoynrMessageFactoryTest, testTtlUpliftWithLargeTtl)
{
    const JoynrTimePoint expectedTimePoint = DispatcherUtils::getMaxAbsoluteTime();

    std::int64_t ttl;
    MessagingQos messagingQos;
    JoynrMessage message;
    JoynrTimePoint timePoint;

    ttl = INT64_MAX;
    messagingQos.setTtl(ttl);
    message = factoryWithTtlUplift.createRequest(senderID, receiverID, messagingQos, request);
    timePoint = message.getHeaderExpiryDate();
    EXPECT_EQ(expectedTimePoint, timePoint);

    // TODO uncomment failing tests
    // after overflow checks in DispatcherUtils.convertTtlToAbsoluteTime are fixed

//    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count();
//    messagingQos.setTtl(ttl);
//    message = factoryWithTtlUplift.createRequest(senderID, receiverID, messagingQos, request);
//    timePoint = message.getHeaderExpiryDate();
//    EXPECT_EQ(expectedTimePoint, timePoint);

//    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count() - ttlUplift;
//    messagingQos.setTtl(ttl);
//    message = factoryWithTtlUplift.createRequest(senderID, receiverID, messagingQos, request);
//    timePoint = message.getHeaderExpiryDate();
//    EXPECT_EQ(expectedTimePoint, timePoint);

    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count()
            - ttlUplift
            - std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();
    messagingQos.setTtl(ttl);
    message = factoryWithTtlUplift.createRequest(senderID, receiverID, messagingQos, request);
    timePoint = message.getHeaderExpiryDate();
    EXPECT_EQ(expectedTimePoint, timePoint);

//    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count()
//            - ttlUplift
//            - std::chrono::time_point_cast<std::chrono::milliseconds>(
//                std::chrono::system_clock::now()).time_since_epoch().count() + 1;
//    messagingQos.setTtl(ttl);
//    message = factoryWithTtlUplift.createRequest(senderID, receiverID, messagingQos, request);
//    timePoint = message.getHeaderExpiryDate();
//    EXPECT_EQ(expectedTimePoint, timePoint);
}

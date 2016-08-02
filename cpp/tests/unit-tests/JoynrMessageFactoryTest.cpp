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
#include <gtest/gtest.h>
#include "joynr/JoynrMessageFactory.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/MessagingQos.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include <chrono>
#include <cstdint>

using namespace joynr;

class JoynrMessageFactoryTest : public ::testing::Test
{
public:
    JoynrMessageFactoryTest()
            : messageFactory(),
              senderID(),
              receiverID(),
              requestReplyID(),
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
        request.addParam(Variant::make<int>(42), "java.lang.Integer");
        request.addParam(Variant::make<std::string>("value"), "java.lang.String");
        oneWayRequest.setMethodName("methodName");
        oneWayRequest.addParam(Variant::make<int>(42), "java.lang.Integer");
        oneWayRequest.addParam(Variant::make<std::string>("value"), "java.lang.String");
        reply.setRequestReplyId(requestReplyID);
        std::vector<Variant> response;
        response.push_back(Variant::make<std::string>("response"));
        reply.setResponse(std::move(response));

        std::string subscriptionId("subscriptionTestId");
        subscriptionPublication.setSubscriptionId(subscriptionId);
        response.clear();
        response.push_back(Variant::make<std::string>("publication"));
        subscriptionPublication.setResponse(response);
    }
    void TearDown()
    {
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
        expectedPayloadStream << R"("methodName": "methodName",)";
        expectedPayloadStream << R"("paramDatatypes": ["java.lang.Integer","java.lang.String"],)";
        expectedPayloadStream << R"("params": [42,"value"],)";
        expectedPayloadStream << R"("requestReplyId": ")" << request.getRequestReplyId() << R"("})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkOneWayRequest(const JoynrMessage& joynrMessage)
    {
        // TODO create expected string from params and methodName
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.OneWayRequest",)";
        expectedPayloadStream << R"("methodName": "methodName",)";
        expectedPayloadStream << R"("paramDatatypes": ["java.lang.Integer","java.lang.String"],)";
        expectedPayloadStream << R"("params": [42,"value"])";
        expectedPayloadStream << R"(})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkReply(const JoynrMessage& joynrMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.Reply",)";
        expectedPayloadStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
        expectedPayloadStream << R"("response": ["response"]})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

    void checkSubscriptionPublication(const JoynrMessage& joynrMessage)
    {
        std::stringstream expectedPayloadStream;
        expectedPayloadStream << R"({"_typeName":"joynr.SubscriptionPublication",)";
        expectedPayloadStream << R"("subscriptionId": ")"
                              << subscriptionPublication.getSubscriptionId() << R"(",)";
        expectedPayloadStream << R"("response": ["publication"]})";
        std::string expectedPayload = expectedPayloadStream.str();
        EXPECT_EQ(expectedPayload, joynrMessage.getPayload());
    }

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

TEST_F(JoynrMessageFactoryTest, createSubscriptionRequest)
{
    Variant subscriptionQos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos());
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(subscriptionQos);
    JoynrMessage joynrMessage = messageFactory.createSubscriptionRequest(
            senderID, receiverID, qos, subscriptionRequest);
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_EQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST, joynrMessage.getType());
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
    std::vector<Variant> params;
    params.push_back(Variant::make<std::string>("test"));
    request.setMethodName("methodName");
    request.setParams(params);

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

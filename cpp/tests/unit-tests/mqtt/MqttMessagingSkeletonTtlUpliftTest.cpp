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
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cluster-controller/mqtt/MqttMessagingSkeleton.h"

#include "joynr/DispatcherUtils.h"
#include "joynr/Request.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "tests/utils/MockObjects.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Property;

using namespace ::testing;
using namespace joynr;

class MqttMessagingSkeletonTtlUpliftTest : public ::testing::Test {
public:
    MqttMessagingSkeletonTtlUpliftTest() :
        singleThreadedIOService(),
        mockMessageRouter(singleThreadedIOService.getIOService()),
        senderID("senderId"),
        receiverID("receiverId"),
        ttlUpliftMs(10000),
        isLocalMessage(false)
    {
        singleThreadedIOService.start();
    }

    void SetUp(){
        joynr::system::RoutingTypes::MqttAddress replyAddress;
        replyAddressSerialized = joynr::serializer::serializeToJson(replyAddress);
    }

protected:
    void transmitCallsAddNextHop();
    SingleThreadedIOService singleThreadedIOService;
    MockMessageRouter mockMessageRouter;
    JoynrMessageFactory messageFactory;
    std::string replyAddressSerialized;
    std::string senderID;
    std::string receiverID;
    std::uint64_t ttlUpliftMs;
    const bool isLocalMessage;
};

TEST_F(MqttMessagingSkeletonTtlUpliftTest, testDefaultTtlUplift) {
    std::int64_t ttlMs = 1024;
    MessagingQos qos(ttlMs);
    Request request;
    JoynrMessage message = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request,
                isLocalMessage);
    message.setHeaderReplyAddress(replyAddressSerialized);

    JoynrTimePoint expectedExpiryDate = message.getHeaderExpiryDate();

    EXPECT_CALL(mockMessageRouter, route(Property(&JoynrMessage::getHeaderExpiryDate, Eq(expectedExpiryDate)),_));

    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr);
    std::string serializedMessage = serializer::serializeToJson(message);
    mqttMessagingSkeleton.onTextMessageReceived(serializedMessage);
}

TEST_F(MqttMessagingSkeletonTtlUpliftTest, testTtlUplift) {
    std::int64_t ttlMs = 1024;
    MessagingQos qos(ttlMs);
    Request request;
    JoynrMessage message = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request,
                isLocalMessage);
    message.setHeaderReplyAddress(replyAddressSerialized);

    JoynrTimePoint expectedExpiryDate = message.getHeaderExpiryDate() + std::chrono::milliseconds(ttlUpliftMs);

    EXPECT_CALL(mockMessageRouter, route(Property(&JoynrMessage::getHeaderExpiryDate, Eq(expectedExpiryDate)),_));

    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr, ttlUpliftMs);
    std::string serializedMessage = serializer::serializeToJson(message);
    mqttMessagingSkeleton.onTextMessageReceived(serializedMessage);
}

TEST_F(MqttMessagingSkeletonTtlUpliftTest, testTtlUpliftWithLargeTtl) {
    const JoynrTimePoint maxAbsoluteTime = DispatcherUtils::getMaxAbsoluteTime();
    std::int64_t ttlMs = 1024;
    MessagingQos qos(ttlMs);
    Request request;
    JoynrMessage message = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request,
                isLocalMessage);
    message.setHeaderReplyAddress(replyAddressSerialized);

    JoynrTimePoint expiryDate = maxAbsoluteTime - std::chrono::milliseconds(ttlUpliftMs / 2);
    message.setHeaderExpiryDate(expiryDate);
    JoynrTimePoint expectedExpiryDate = maxAbsoluteTime;
    EXPECT_CALL(mockMessageRouter, route(Property(&JoynrMessage::getHeaderExpiryDate, Eq(expectedExpiryDate)),_));
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr, ttlUpliftMs);
    std::string serializedMessage = serializer::serializeToJson(message);
    mqttMessagingSkeleton.onTextMessageReceived(serializedMessage);

    expiryDate = maxAbsoluteTime - std::chrono::milliseconds(ttlUpliftMs);
    message.setHeaderExpiryDate(expiryDate);
    expectedExpiryDate = maxAbsoluteTime;
    EXPECT_CALL(mockMessageRouter, route(Property(&JoynrMessage::getHeaderExpiryDate, Eq(expectedExpiryDate)),_));
    serializedMessage = serializer::serializeToJson(message);
    mqttMessagingSkeleton.onTextMessageReceived(serializedMessage);

    expiryDate = maxAbsoluteTime - std::chrono::milliseconds(ttlUpliftMs + 1);
    message.setHeaderExpiryDate(expiryDate);
    expectedExpiryDate = message.getHeaderExpiryDate() + std::chrono::milliseconds(ttlUpliftMs);
    EXPECT_CALL(mockMessageRouter, route(Property(&JoynrMessage::getHeaderExpiryDate, Eq(expectedExpiryDate)),_));
    serializedMessage = serializer::serializeToJson(message);
    mqttMessagingSkeleton.onTextMessageReceived(serializedMessage);
}

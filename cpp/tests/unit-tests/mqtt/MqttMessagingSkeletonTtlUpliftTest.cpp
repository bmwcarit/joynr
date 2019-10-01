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
#include <chrono>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/MqttMessagingSkeleton.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Request.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/MessagingQos.h"
#include "joynr/Settings.h"
#include "joynr/TimePoint.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageRouter.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Property;

using namespace ::testing;
using namespace joynr;

class MqttMessagingSkeletonTtlUpliftTest : public ::testing::Test
{
public:
    MqttMessagingSkeletonTtlUpliftTest()
            : singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              mockMessageRouter(
                      std::make_shared<MockMessageRouter>(singleThreadedIOService->getIOService())),
              senderID("senderId"),
              receiverID("receiverId"),
              ttlUpliftMs(10000),
              isLocalMessage(false),
              settings(),
              ccSettings(settings)
    {
        singleThreadedIOService->start();
    }

    ~MqttMessagingSkeletonTtlUpliftTest()
    {
        singleThreadedIOService->stop();
    }

    void SetUp()
    {
        joynr::system::RoutingTypes::MqttAddress replyAddress;
        replyAddressSerialized = joynr::serializer::serializeToJson(replyAddress);
    }

protected:
    void transmitCallsAddNextHop();
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    MutableMessageFactory messageFactory;
    std::string replyAddressSerialized;
    std::string senderID;
    std::string receiverID;
    std::uint64_t ttlUpliftMs;
    const bool isLocalMessage;
    Settings settings;
    ClusterControllerSettings ccSettings;
};

TEST_F(MqttMessagingSkeletonTtlUpliftTest, testDefaultTtlUplift)
{
    std::uint64_t ttlMs = 1024;
    MessagingQos qos(ttlMs);
    Request request;
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    mutableMessage.setReplyTo(replyAddressSerialized);

    const TimePoint expectedExpiryDate = mutableMessage.getExpiryDate();

    EXPECT_CALL(*mockMessageRouter, route(MessageHasExpiryDate(expectedExpiryDate), _));

    MqttMessagingSkeleton mqttMessagingSkeleton(
            mockMessageRouter, nullptr, ccSettings.getMqttMulticastTopicPrefix(), "testGbid");
    std::unique_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
    mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
}

TEST_F(MqttMessagingSkeletonTtlUpliftTest, DISABLED_testTtlUplift)
{
    std::uint64_t ttlMs = 1024;
    MessagingQos qos(ttlMs);
    Request request;
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    mutableMessage.setReplyTo(replyAddressSerialized);

    const TimePoint expectedExpiryDate =
            mutableMessage.getExpiryDate() + std::chrono::milliseconds(ttlUpliftMs);

    EXPECT_CALL(*mockMessageRouter, route(MessageHasExpiryDate(expectedExpiryDate), _));

    MqttMessagingSkeleton mqttMessagingSkeleton(
            mockMessageRouter, nullptr, ccSettings.getMqttMulticastTopicPrefix(), "testGbid", ttlUpliftMs);
    std::unique_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
    mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
}

TEST_F(MqttMessagingSkeletonTtlUpliftTest, DISABLED_testTtlUpliftWithLargeTtl)
{
    const TimePoint maxAbsoluteTime = TimePoint::max();
    std::uint64_t ttlMs = 1024;
    MessagingQos qos(ttlMs);
    Request request;
    MutableMessage mutableMessage =
            messageFactory.createRequest(senderID, receiverID, qos, request, isLocalMessage);
    mutableMessage.setReplyTo(replyAddressSerialized);

    TimePoint expiryDate = maxAbsoluteTime - std::chrono::milliseconds(ttlUpliftMs / 2);
    mutableMessage.setExpiryDate(expiryDate);
    TimePoint expectedExpiryDate = maxAbsoluteTime;

    EXPECT_CALL(*mockMessageRouter, route(MessageHasExpiryDate(expectedExpiryDate), _));

    MqttMessagingSkeleton mqttMessagingSkeleton(
            mockMessageRouter, nullptr, ccSettings.getMqttMulticastTopicPrefix(), "testGbid", ttlUpliftMs);
    {
        std::unique_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
        mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
    }

    expiryDate = maxAbsoluteTime - std::chrono::milliseconds(ttlUpliftMs);
    mutableMessage.setExpiryDate(expiryDate);
    expectedExpiryDate = maxAbsoluteTime;

    EXPECT_CALL(*mockMessageRouter, route(MessageHasExpiryDate(expectedExpiryDate), _));

    {
        std::unique_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
        mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
    }

    expiryDate = maxAbsoluteTime - std::chrono::milliseconds(ttlUpliftMs + 1);
    mutableMessage.setExpiryDate(expiryDate);
    expectedExpiryDate = mutableMessage.getExpiryDate() + std::chrono::milliseconds(ttlUpliftMs);

    EXPECT_CALL(*mockMessageRouter, route(MessageHasExpiryDate(expectedExpiryDate), _));

    {
        std::unique_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
        mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
    }
}

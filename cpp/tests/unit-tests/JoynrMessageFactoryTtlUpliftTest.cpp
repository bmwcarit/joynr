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
#include <cstdint>
#include <string>

#include "tests/utils/Gtest.h"

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/TimePoint.h"

using namespace joynr;

class JoynrMessageFactoryTtlUpliftTest : public ::testing::Test
{
public:
    JoynrMessageFactoryTtlUpliftTest()
            : _messageFactory(0),
              _senderID("senderId"),
              _receiverID("receiverID"),
              _ttl(1000),
              _ttlUplift(10000),
              _upliftedTtl(_ttl + static_cast<std::int64_t>(_ttlUplift)),
              _isLocalMessage(true),
              _messagingQos(),
              _factoryWithTtlUplift(_ttlUplift)
    {
        _messagingQos.setTtl(static_cast<std::uint64_t>(_ttl));
    }

    void checkMessageExpiryDate(const MutableMessage& message, const std::int64_t expectedTtl);

protected:
    ADD_LOGGER(JoynrMessageFactoryTtlUpliftTest)
    MutableMessageFactory _messageFactory;
    std::string _senderID;
    std::string _receiverID;

    const std::int64_t _ttl;
    const std::uint64_t _ttlUplift;
    const std::int64_t _upliftedTtl;
    const bool _isLocalMessage;
    MessagingQos _messagingQos;
    MutableMessageFactory _factoryWithTtlUplift;
};

void JoynrMessageFactoryTtlUpliftTest::checkMessageExpiryDate(const MutableMessage& message,
                                                              const std::int64_t expectedTtl)
{
    const std::int64_t tolerance = 50;
    std::int64_t actualTtl = message.getExpiryDate().relativeFromNow().count();
    std::int64_t diff = expectedTtl - actualTtl;
    EXPECT_GE(diff, 0);
    EXPECT_LE(std::abs(diff), tolerance)
            << "ttl from expiryDate " + std::to_string(actualTtl) + "ms differs " +
                       std::to_string(diff) + "ms (more than " + std::to_string(tolerance) +
                       "ms) from the expected ttl " + std::to_string(expectedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testDefaultTtlUplift)
{
    Request request;
    MutableMessage message = _messageFactory.createRequest(
            _senderID, _receiverID, _messagingQos, request, _isLocalMessage);

    checkMessageExpiryDate(message, _ttl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_Request)
{
    Request request;
    MutableMessage message = _factoryWithTtlUplift.createRequest(
            _senderID, _receiverID, _messagingQos, request, _isLocalMessage);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_Reply_noUplift)
{
    Reply reply;
    MutableMessage message =
            _factoryWithTtlUplift.createReply(_senderID, _receiverID, _messagingQos, {}, reply);

    checkMessageExpiryDate(message, _ttl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_OneWayRequest)
{
    OneWayRequest oneWayRequest;
    MutableMessage message = _factoryWithTtlUplift.createOneWayRequest(
            _senderID, _receiverID, _messagingQos, oneWayRequest, _isLocalMessage);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_MulticastPublication)
{
    MulticastPublication publication;
    MutableMessage message =
            _factoryWithTtlUplift.createMulticastPublication(_senderID, _messagingQos, publication);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_SubscriptionPublication)
{
    SubscriptionPublication subscriptionPublication;
    MutableMessage message = _factoryWithTtlUplift.createSubscriptionPublication(
            _senderID, _receiverID, _messagingQos, subscriptionPublication);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_SubscriptionRequest)
{
    SubscriptionRequest request;
    MutableMessage message = _factoryWithTtlUplift.createSubscriptionRequest(
            _senderID, _receiverID, _messagingQos, request, _isLocalMessage);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_MulticastSubscriptionRequest)
{
    MulticastSubscriptionRequest request;
    MutableMessage message = _factoryWithTtlUplift.createMulticastSubscriptionRequest(
            _senderID, _receiverID, _messagingQos, request, _isLocalMessage);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_BroadcastSubscriptionRequest)
{
    BroadcastSubscriptionRequest request;
    MutableMessage message = _factoryWithTtlUplift.createBroadcastSubscriptionRequest(
            _senderID, _receiverID, _messagingQos, request, _isLocalMessage);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_SubscriptionReply_noUplift)
{
    SubscriptionReply reply;
    MutableMessage message = _factoryWithTtlUplift.createSubscriptionReply(
            _senderID, _receiverID, _messagingQos, reply);

    checkMessageExpiryDate(message, _ttl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUplift_SubscriptionStop)
{
    SubscriptionStop subscriptionStop;
    MutableMessage message = _factoryWithTtlUplift.createSubscriptionStop(
            _senderID, _receiverID, _messagingQos, subscriptionStop);

    checkMessageExpiryDate(message, _upliftedTtl);
}

TEST_F(JoynrMessageFactoryTtlUpliftTest, testTtlUpliftWithLargeTtl)
{
    const TimePoint expectedTimePoint = TimePoint::max();
    Request request;

    std::int64_t localTtl;
    MessagingQos localMessagingQos;
    MutableMessage message;

    localTtl = INT64_MAX;
    localMessagingQos.setTtl(static_cast<std::uint64_t>(localTtl));
    message = _factoryWithTtlUplift.createRequest(
            _senderID, _receiverID, localMessagingQos, request, _isLocalMessage);
    TimePoint timePoint = message.getExpiryDate();
    EXPECT_EQ(expectedTimePoint, timePoint)
            << "expected timepoint: " + std::to_string(expectedTimePoint.toMilliseconds()) +
                       " actual: " + std::to_string(timePoint.toMilliseconds());

    // TODO uncomment failing tests
    // after overflow checks in DispatcherUtils.convertTtlToAbsoluteTime are fixed

    //    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count();
    //    messagingQos.setTtl(ttl);
    //    message = _factoryWithTtlUplift.createRequest(_senderID, _receiverID, messagingQos,
    //    request); timePoint = message.getHeaderExpiryDate(); EXPECT_EQ(expectedTimePoint,
    //    timePoint);

    //    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count() - ttlUplift;
    //    messagingQos.setTtl(ttl);
    //    message = _factoryWithTtlUplift.createRequest(_senderID, _receiverID, messagingQos,
    //    request); timePoint = message.getHeaderExpiryDate(); EXPECT_EQ(expectedTimePoint,
    //    timePoint);

    auto now = TimePoint::now();
    localTtl = (TimePoint::max() - std::chrono::milliseconds(_ttlUplift)).relativeFromNow().count();
    localMessagingQos.setTtl(static_cast<std::uint64_t>(localTtl));
    message = _factoryWithTtlUplift.createRequest(
            _senderID, _receiverID, localMessagingQos, request, _isLocalMessage);
    //    timePoint = message.getHeaderExpiryDate();
    //    EXPECT_EQ(expectedTimePoint, timePoint) << "expected timepoint: "
    //                                               +
    //                                               std::to_string(expectedTimePoint.time_since_epoch().count())
    //                                               + " actual: "
    //                                               +
    //                                               std::to_string(timePoint.time_since_epoch().count());
    checkMessageExpiryDate(message, (expectedTimePoint - now).count());

    //    ttl = DispatcherUtils::getMaxAbsoluteTime().time_since_epoch().count()
    //            - ttlUplift
    //            - std::chrono::time_point_cast<std::chrono::milliseconds>(
    //                std::chrono::system_clock::now()).time_since_epoch().count() + 1;
    //    messagingQos.setTtl(ttl);
    //    message = _factoryWithTtlUplift.createRequest(_senderID, _receiverID, messagingQos,
    //    request); timePoint = message.getHeaderExpiryDate(); EXPECT_EQ(expectedTimePoint,
    //    timePoint);
}

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

#include <chrono>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/Message.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/Request.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"

#include "libjoynr/uds/UdsLibJoynrMessagingSkeleton.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageRouter.h"

using namespace ::testing;
using namespace joynr;

class UdsLibJoynrMessagingSkeletonTest : public ::testing::Test
{
public:
    UdsLibJoynrMessagingSkeletonTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _mockMessageRouter(
                      std::make_shared<MockMessageRouter>(_singleThreadedIOService->getIOService())),
              _messageFactory(),
              _mutableMessage(),
              _senderID("senderID"),
              _receiverID("receiverId"),
              _qosSettings(),
              _isLocalMessage(true),
              _udsAddress(std::make_shared<joynr::system::RoutingTypes::UdsAddress>("somepath"))
    {
    }

    ~UdsLibJoynrMessagingSkeletonTest()
    {
    }

    void SetUp()
    {
        auto postFix = "_" + util::createUuid();
        _senderID += postFix;
        _receiverID += postFix;
        _qosSettings = MessagingQos(456000);
        Request request;
        request.setMethodName("methodName");
        request.setParams(42, std::string("value"));
        std::vector<std::string> paramDatatypes;
        paramDatatypes.push_back("Integer");
        paramDatatypes.push_back("String");
        request.setParamDatatypes(paramDatatypes);

        _mutableMessage = _messageFactory.createRequest(
                _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
        _serializedUdsAddress = joynr::serializer::serializeToJson(*_udsAddress);
        _mutableMessage.setReplyTo(_serializedUdsAddress);
    }

protected:
    void transmitDoesNotSetIsReceivedFromGlobal();
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    MutableMessageFactory _messageFactory;
    MutableMessage _mutableMessage;
    std::string _senderID;
    std::string _receiverID;
    MessagingQos _qosSettings;
    const bool _isLocalMessage;
    std::shared_ptr<joynr::system::RoutingTypes::UdsAddress> _udsAddress;
    std::string _serializedUdsAddress;
};

TEST_F(UdsLibJoynrMessagingSkeletonTest, transmitTest)
{
    UdsLibJoynrMessagingSkeleton udsLibJoynrMessagingSkeleton(_mockMessageRouter);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_CALL(*_mockMessageRouter, route(immutableMessage, _)).Times(1);

    auto onFailure =
            [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; };
    udsLibJoynrMessagingSkeleton.transmit(immutableMessage, onFailure);
}

TEST_F(UdsLibJoynrMessagingSkeletonTest, onMessageReceivedTest)
{
    UdsLibJoynrMessagingSkeleton udsLibJoynrMessagingSkeleton(_mockMessageRouter);
    std::unique_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();

    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(MessageHasType(_mutableMessage.getType()),
                            ImmutableMessageHasPayload(_mutableMessage.getPayload())),
                      _)).Times(1);

    smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
    std::string creator("creator");
    udsLibJoynrMessagingSkeleton.onMessageReceived(std::move(serializedMessage), creator);
}

TEST_F(UdsLibJoynrMessagingSkeletonTest, transmitSetsReceivedFromGlobalForMulticastPublications)
{
    MulticastPublication publication;
    _mutableMessage = _messageFactory.createMulticastPublication(_senderID, _qosSettings, publication);

    UdsLibJoynrMessagingSkeleton udsLibJoynrMessagingSkeleton(_mockMessageRouter);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_FALSE(immutableMessage->isReceivedFromGlobal());
    auto onFailure =
            [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; };
    EXPECT_CALL(*_mockMessageRouter, route(immutableMessage, _)).Times(1);
    udsLibJoynrMessagingSkeleton.transmit(immutableMessage, onFailure);
    EXPECT_TRUE(immutableMessage->isReceivedFromGlobal());
}

TEST_F(UdsLibJoynrMessagingSkeletonTest, transmitDoesNotSetReceivedFromGlobalForRequests)
{
    Request request;
    _mutableMessage = _messageFactory.createRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedUdsAddress);
    transmitDoesNotSetIsReceivedFromGlobal();
}

TEST_F(UdsLibJoynrMessagingSkeletonTest, transmitDoesNotSetReceivedFromGlobalForSubscriptionRequests)
{
    SubscriptionRequest request;
    _mutableMessage = _messageFactory.createSubscriptionRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedUdsAddress);
    transmitDoesNotSetIsReceivedFromGlobal();
}

TEST_F(UdsLibJoynrMessagingSkeletonTest, transmitDoesNotSetIsReceivedFromGlobalForBroadcastSubscriptionRequests)
{
    BroadcastSubscriptionRequest request;
    _mutableMessage = _messageFactory.createBroadcastSubscriptionRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedUdsAddress);
    transmitDoesNotSetIsReceivedFromGlobal();
}

TEST_F(UdsLibJoynrMessagingSkeletonTest, transmitDoesNotSetIsReceivedFromGlobalForMulticastSubscriptionRequests)
{
    MulticastSubscriptionRequest request;
    _mutableMessage = _messageFactory.createMulticastSubscriptionRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedUdsAddress);
    transmitDoesNotSetIsReceivedFromGlobal();
}

void UdsLibJoynrMessagingSkeletonTest::transmitDoesNotSetIsReceivedFromGlobal()
{
    UdsLibJoynrMessagingSkeleton udsLibJoynrMessagingSkeleton(_mockMessageRouter);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_FALSE(immutableMessage->isReceivedFromGlobal());
    auto onFailure =
            [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; };
    EXPECT_CALL(*_mockMessageRouter, route(immutableMessage, _)).Times(1);
    udsLibJoynrMessagingSkeleton.transmit(immutableMessage, onFailure);
    EXPECT_FALSE(immutableMessage->isReceivedFromGlobal());
}

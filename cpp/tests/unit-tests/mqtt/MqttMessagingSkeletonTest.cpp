/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
#include <functional>
#include <memory>
#include <string>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/BrokerUrl.h"
#include "joynr/MqttMessagingSkeleton.h"
#include "joynr/MqttReceiver.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/Request.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageRouter.h"

using ::testing::_;
using ::testing::A;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Pointee;
using ::testing::Property;

using namespace ::testing;
using namespace joynr;

class MockMqttReceiver : public joynr::MqttReceiver
{
public:
    MockMqttReceiver(const MessagingSettings& messagingSettings,
                     const ClusterControllerSettings& ccSettings,
                     joynr::BrokerUrl brokerUrl,
                     std::chrono::seconds mqttKeepAliveTimeSeconds,
                     std::chrono::seconds mqttReconnectDelayTimeSeconds,
                     std::chrono::seconds mqttReconnectMaxDelayTimeSeconds,
                     bool isMqttExponentialBackoffEnabled,
                     std::string gbid,
                     std::string receiverId)
            : MqttReceiver(std::make_shared<MosquittoConnection>(ccSettings,
                                                                 brokerUrl,
                                                                 mqttKeepAliveTimeSeconds,
                                                                 mqttReconnectDelayTimeSeconds,
                                                                 mqttReconnectMaxDelayTimeSeconds,
                                                                 isMqttExponentialBackoffEnabled,
                                                                 receiverId,
                                                                 "gbid"),
                           messagingSettings,
                           "channelId",
                           gbid,
                           ccSettings.getMqttMulticastTopicPrefix())
    {
    }
    MOCK_METHOD1(subscribeToTopic, void(const std::string& topic));
    MOCK_METHOD1(unsubscribeFromTopic, void(const std::string& topic));
};

class MqttMessagingSkeletonTest : public ::testing::Test
{
public:
    MqttMessagingSkeletonTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _mockMessageRouter(std::make_shared<MockMessageRouter>(
                      _singleThreadedIOService->getIOService())),
              _isLocalMessage(false),
              _settings(),
              _ccSettings(_settings),
              _brokerUrl("mqtt://testBrokerHost:1883"),
              _mqttKeepAliveTimeSeconds(1),
              _mqttReconnectDelayTimeSeconds(1),
              _mqttReconnectMaxDelayTimeSeconds(1),
              _isMqttExponentialBackoffEnabled(false),
              _receiverId("receiverId"),
              _testGbid("testGbid"),
              _mqttAddress(std::make_shared<joynr::system::RoutingTypes::MqttAddress>("fakegbid",
                                                                                      "testtopic"))

    {
        _singleThreadedIOService->start();
    }

    ~MqttMessagingSkeletonTest()
    {
        _singleThreadedIOService->stop();
    }

    void SetUp()
    {
        // create a fake message
        std::string postFix;

        postFix = "_" + util::createUuid();
        _senderID = "senderId" + postFix;
        _receiverID = "receiverID" + postFix;
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
        _serializedMqttAddress = joynr::serializer::serializeToJson(*_mqttAddress);
        _mutableMessage.setReplyTo(_serializedMqttAddress);
    }

protected:
    void transmitSetsIsReceivedFromGlobal();
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    MutableMessageFactory _messageFactory;
    MutableMessage _mutableMessage;
    std::string _senderID;
    std::string _receiverID;
    MessagingQos _qosSettings;
    const bool _isLocalMessage;
    Settings _settings;
    ClusterControllerSettings _ccSettings;

    joynr::BrokerUrl _brokerUrl;
    std::chrono::seconds _mqttKeepAliveTimeSeconds;
    std::chrono::seconds _mqttReconnectDelayTimeSeconds;
    std::chrono::seconds _mqttReconnectMaxDelayTimeSeconds;
    bool _isMqttExponentialBackoffEnabled;
    std::string _receiverId;
    std::string _testGbid;

    std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> _mqttAddress;
    std::string _serializedMqttAddress;
};

MATCHER_P(pointerToMqttAddressWithChannelId, channelId, "")
{
    if (arg == nullptr) {
        return false;
    }
    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> mqttAddress =
            std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(arg);
    if (mqttAddress == nullptr) {
        return false;
    }
    return mqttAddress->getTopic() == channelId;
}

MATCHER(isPointerToMqttAddress, "")
{
    if (arg == nullptr) {
        return false;
    }
    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> mqttAddress =
            std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(arg);
    if (mqttAddress == nullptr) {
        return false;
    }
    return true;
}

TEST_F(MqttMessagingSkeletonTest, transmitTest)
{
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_CALL(*_mockMessageRouter, route(immutableMessage, _)).Times(1);
    MqttMessagingSkeleton mqttMessagingSkeleton(
            _mockMessageRouter, nullptr, _ccSettings.getMqttMulticastTopicPrefix(), _testGbid);

    auto onFailure = [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; };
    mqttMessagingSkeleton.transmit(immutableMessage, onFailure);
}

TEST_F(MqttMessagingSkeletonTest, transmitTestWithMqttReplyToAddress)
{
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_CALL(*_mockMessageRouter, route(immutableMessage, _)).Times(1);
    EXPECT_CALL(
            *_mockMessageRouter,
            addNextHop(
                    Eq(immutableMessage->getSender()),
                    AllOf(Property(
                                  &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                                  WhenDynamicCastTo<
                                          const joynr::system::RoutingTypes::MqttAddress*>(
                                          Pointee(AllOf(Property(&joynr::system::RoutingTypes::
                                                                         MqttAddress::getBrokerUri,
                                                                 Eq(_testGbid)),
                                                        Property(&joynr::system::RoutingTypes::
                                                                         MqttAddress::getTopic,
                                                                 Eq(_mqttAddress->getTopic())))))),
                          isPointerToMqttAddress()),
                    Eq(true), Eq(TimePoint::max().toMilliseconds()), Eq(false), _, _))
            .Times(1);

    MqttMessagingSkeleton mqttMessagingSkeleton(
            _mockMessageRouter, nullptr, _ccSettings.getMqttMulticastTopicPrefix(), _testGbid);
    auto onFailure = [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; };
    mqttMessagingSkeleton.transmit(immutableMessage, onFailure);
}

TEST_F(MqttMessagingSkeletonTest, transmitTestWithBrokenReplyToAddress)
{
    EXPECT_CALL(*_mockMessageRouter, route(_, _)).Times(0);
    EXPECT_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _)).Times(0);
    MqttMessagingSkeleton mqttMessagingSkeleton(
            _mockMessageRouter, nullptr, _ccSettings.getMqttMulticastTopicPrefix(), _testGbid);
    _mutableMessage.setReplyTo("thisisinvalid::==");
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    auto semaphore = std::make_shared<joynr::Semaphore>(0);
    auto onFailure = [semaphore](const exceptions::JoynrRuntimeException&) {
        SUCCEED() << "onFailure called";
        semaphore->notify();
    };

    mqttMessagingSkeleton.transmit(immutableMessage, onFailure);
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(10)));
}

void MqttMessagingSkeletonTest::transmitSetsIsReceivedFromGlobal()
{
    MqttMessagingSkeleton mqttMessagingSkeleton(
            _mockMessageRouter, nullptr, _ccSettings.getMqttMulticastTopicPrefix(), _testGbid);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_FALSE(immutableMessage->isReceivedFromGlobal());
    auto onFailure = [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; };
    mqttMessagingSkeleton.transmit(immutableMessage, onFailure);
    EXPECT_TRUE(immutableMessage->isReceivedFromGlobal());
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsReceivedFromGlobalForMulticastPublications)
{
    MulticastPublication publication;
    _mutableMessage =
            _messageFactory.createMulticastPublication(_senderID, _qosSettings, publication);
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsReceivedFromGlobalForRequests)
{
    Request request;
    _mutableMessage = _messageFactory.createRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedMqttAddress);
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsReceivedFromGlobalForSubscriptionRequests)
{
    SubscriptionRequest request;
    _mutableMessage = _messageFactory.createSubscriptionRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedMqttAddress);
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsIsReceivedFromGlobalForBroadcastSubscriptionRequests)
{
    BroadcastSubscriptionRequest request;
    _mutableMessage = _messageFactory.createBroadcastSubscriptionRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedMqttAddress);
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsIsReceivedFromGlobalForMulticastSubscriptionRequests)
{
    MulticastSubscriptionRequest request;
    _mutableMessage = _messageFactory.createMulticastSubscriptionRequest(
            _senderID, _receiverID, _qosSettings, request, _isLocalMessage);
    _mutableMessage.setReplyTo(_serializedMqttAddress);
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, onMessageReceivedTest)
{
    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(MessageHasType(_mutableMessage.getType()),
                            ImmutableMessageHasPayload(_mutableMessage.getPayload())),
                      _))
            .Times(1);
    MqttMessagingSkeleton mqttMessagingSkeleton(
            _mockMessageRouter, nullptr, _ccSettings.getMqttMulticastTopicPrefix(), _testGbid);
    std::unique_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();

    smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
    mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
}

TEST_F(MqttMessagingSkeletonTest, registerMulticastSubscription_subscribesToMqttTopic)
{
    std::string multicastId = "multicastId";
    Settings settings;
    ClusterControllerSettings ccSettings(settings);
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings,
                                                               ccSettings,
                                                               _brokerUrl,
                                                               _mqttKeepAliveTimeSeconds,
                                                               _mqttReconnectDelayTimeSeconds,
                                                               _mqttReconnectMaxDelayTimeSeconds,
                                                               _isMqttExponentialBackoffEnabled,
                                                               _testGbid,
                                                               _receiverId);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId));
    MqttMessagingSkeleton mqttMessagingSkeleton(
            _mockMessageRouter, mockMqttReceiver, "", _testGbid);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, registerMulticastSubscription_subscribesToMqttTopicOnlyOnce)
{
    std::string multicastId = "multicastId";
    MessagingSettings messagingSettings(_settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings,
                                                               _ccSettings,
                                                               _brokerUrl,
                                                               _mqttKeepAliveTimeSeconds,
                                                               _mqttReconnectDelayTimeSeconds,
                                                               _mqttReconnectMaxDelayTimeSeconds,
                                                               _isMqttExponentialBackoffEnabled,
                                                               _testGbid,
                                                               _receiverId);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    MqttMessagingSkeleton mqttMessagingSkeleton(_mockMessageRouter, mockMqttReceiver,
                                                _ccSettings.getMqttMulticastTopicPrefix(),
                                                _testGbid);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, unregisterMulticastSubscription_unsubscribesFromMqttTopic)
{
    std::string multicastId = "multicastId";
    MessagingSettings messagingSettings(_settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings,
                                                               _ccSettings,
                                                               _brokerUrl,
                                                               _mqttKeepAliveTimeSeconds,
                                                               _mqttReconnectDelayTimeSeconds,
                                                               _mqttReconnectMaxDelayTimeSeconds,
                                                               _isMqttExponentialBackoffEnabled,
                                                               _testGbid,
                                                               _receiverId);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId));

    MqttMessagingSkeleton mqttMessagingSkeleton(_mockMessageRouter, mockMqttReceiver,
                                                _ccSettings.getMqttMulticastTopicPrefix(),
                                                _testGbid);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest,
       unregisterMulticastSubscription_unsubscribesFromMqttTopicOnlyForUnsubscribeOfLastReceiver)
{
    std::string multicastId = "multicastId";
    MessagingSettings messagingSettings(_settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings,
                                                               _ccSettings,
                                                               _brokerUrl,
                                                               _mqttKeepAliveTimeSeconds,
                                                               _mqttReconnectDelayTimeSeconds,
                                                               _mqttReconnectMaxDelayTimeSeconds,
                                                               _isMqttExponentialBackoffEnabled,
                                                               _testGbid,
                                                               _receiverId);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId)).Times(0);
    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId)).Times(1);

    MqttMessagingSkeleton mqttMessagingSkeleton(_mockMessageRouter, mockMqttReceiver,
                                                _ccSettings.getMqttMulticastTopicPrefix(),
                                                _testGbid);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, translateWildcard)
{
    std::map<std::string, std::string> input2expected = {
            {"partion0/partion1", "partion0/partion1"},
            {"partion0/partion1/*", "partion0/partion1/#"},
            {"partion0/partion1/+", "partion0/partion1/+"}};

    for (auto& testCase : input2expected) {
        std::string inputPartition = testCase.first;
        std::string expectedPartition = testCase.second;
        EXPECT_EQ(expectedPartition,
                  MqttMessagingSkeleton::translateMulticastWildcard(inputPartition));
    }
}

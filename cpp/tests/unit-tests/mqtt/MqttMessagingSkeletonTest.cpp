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
#include <functional>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cluster-controller/mqtt/MqttMessagingSkeleton.h"
#include "cluster-controller/mqtt/MqttReceiver.h"

#include "joynr/MessagingSettings.h"
#include "joynr/Request.h"
#include "joynr/Semaphore.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "tests/utils/MockObjects.h"

using ::testing::A;
using ::testing::_;
using ::testing::Eq;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Pointee;

using namespace ::testing;
using namespace joynr;

class MockMqttReceiver : public joynr::MqttReceiver
{
public:
    MockMqttReceiver(const MessagingSettings& settings) :
        MqttReceiver(settings, "channelId", "receiverId")
    {
    }
    MOCK_METHOD1(subscribeToTopic, void(const std::string& topic));
    MOCK_METHOD1(unsubscribeFromTopic, void(const std::string& topic));
};

class MqttMessagingSkeletonTest : public ::testing::Test {
public:
    MqttMessagingSkeletonTest() :
        singleThreadedIOService(),
        mockMessageRouter(singleThreadedIOService.getIOService())
    {
        singleThreadedIOService.start();
    }

    void SetUp(){
        // create a fake message
        std::string postFix;
        std::string senderID;
        std::string receiverID;
        std::string requestID;

        postFix = "_" + util::createUuid();
        senderID = "senderId" + postFix;
        receiverID = "receiverID" + postFix;
        requestID = "requestId" + postFix;
        MessagingQos qosSettings = MessagingQos(456000);
        Request request;
        request.setMethodName("methodName");
        request.setParams(42, std::string("value"));
        std::vector<std::string> paramDatatypes;
        paramDatatypes.push_back("Integer");
        paramDatatypes.push_back("String");
        request.setParamDatatypes(paramDatatypes);

        JoynrMessageFactory messageFactory;
        message = messageFactory.createRequest(
                senderID,
                receiverID,
                qosSettings,
                request
                );
        joynr::system::RoutingTypes::MqttAddress replyAddress;
        replyAddressSerialized = joynr::serializer::serializeToJson(replyAddress);
        message.setHeaderReplyAddress(replyAddressSerialized);
    }

protected:
    SingleThreadedIOService singleThreadedIOService;
    MockMessageRouter mockMessageRouter;
    JoynrMessage message;
    std::string replyAddressSerialized;
};

MATCHER_P(pointerToMqttAddressWithChannelId, channelId, "") {
    if (arg == nullptr) {
        return false;
    }
    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> mqttAddress = std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(arg);
    if (mqttAddress == nullptr) {
        return false;
    }
    return mqttAddress->getTopic() == channelId;
}

TEST_F(MqttMessagingSkeletonTest, transmitTest) {
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr);
    EXPECT_CALL(mockMessageRouter, addNextHop(
        _,
        AnyOf(
            Pointee(A<joynr::system::RoutingTypes::Address>()),
            pointerToMqttAddressWithChannelId(replyAddressSerialized)
        ),
        _)
    ).Times(1);
    EXPECT_CALL(mockMessageRouter, route(message,_)).Times(1);

    auto onFailure = [](const exceptions::JoynrRuntimeException& e) {
        FAIL() << "onFailure called";
    };
    mqttMessagingSkeleton.transmit(message,onFailure);
}

TEST_F(MqttMessagingSkeletonTest, onTextMessageReceivedTest) {
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr);
    std::string serializedMessage = serializer::serializeToJson(message);
    EXPECT_CALL(mockMessageRouter, route(AllOf(Property(&JoynrMessage::getType, Eq(message.getType())),
                                            Property(&JoynrMessage::getPayload, Eq(message.getPayload()))),_)).Times(1);
    mqttMessagingSkeleton.onTextMessageReceived(serializedMessage);
}

TEST_F(MqttMessagingSkeletonTest, registerMulticastSubscription_subscribesToMqttTopic)
{
    std::string multicastId = "multicastId";
    Settings settings;
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId));
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, registerMulticastSubscription_subscribesToMqttTopicOnlyOnce)
{
    std::string multicastId = "multicastId";
    Settings settings;
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, unregisterMulticastSubscription_unsubscribesFromMqttTopic)
{
    std::string multicastId = "multicastId";
    Settings settings;
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);

    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId));
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, unregisterMulticastSubscription_unsubscribesFromMqttTopicOnlyForUnsubscribeOfLastReceiver)
{
    std::string multicastId = "multicastId";
    Settings settings;
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver);
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);

    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId)).Times(0);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
    Mock::VerifyAndClearExpectations(mockMqttReceiver.get());

    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
}

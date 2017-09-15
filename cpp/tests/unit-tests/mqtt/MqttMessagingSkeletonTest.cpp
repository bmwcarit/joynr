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
#include <functional>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "libjoynrclustercontroller/mqtt/MqttMessagingSkeleton.h"
#include "joynr/MqttReceiver.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/MessagingSettings.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/Request.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"

#include "tests/utils/MockObjects.h"
#include "tests/JoynrTest.h"

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
    MockMqttReceiver(const MessagingSettings& messagingSettings, const ClusterControllerSettings& ccSettings) :
        MqttReceiver(std::make_shared<MosquittoConnection>(messagingSettings, ccSettings, "receiverId"), messagingSettings, "channelId", ccSettings.getMqttMulticastTopicPrefix())
    {
    }
    MOCK_METHOD1(subscribeToTopic, void(const std::string& topic));
    MOCK_METHOD1(unsubscribeFromTopic, void(const std::string& topic));
};

class MqttMessagingSkeletonTest : public ::testing::Test {
public:
    MqttMessagingSkeletonTest() :
        singleThreadedIOService(),
        mockMessageRouter(std::make_shared<MockMessageRouter>(singleThreadedIOService.getIOService())),
        isLocalMessage(false),
        settings(),
        ccSettings(settings)
    {
        singleThreadedIOService.start();
    }

    void SetUp(){
        // create a fake message
        std::string postFix;

        postFix = "_" + util::createUuid();
        senderID = "senderId" + postFix;
        receiverID = "receiverID" + postFix;
        qosSettings = MessagingQos(456000);
        Request request;
        request.setMethodName("methodName");
        request.setParams(42, std::string("value"));
        std::vector<std::string> paramDatatypes;
        paramDatatypes.push_back("Integer");
        paramDatatypes.push_back("String");
        request.setParamDatatypes(paramDatatypes);

        mutableMessage = messageFactory.createRequest(
                senderID,
                receiverID,
                qosSettings,
                request,
                isLocalMessage
                );
    }

protected:
    void transmitSetsIsReceivedFromGlobal();
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    MutableMessageFactory messageFactory;
    MutableMessage mutableMessage;
    std::string senderID;
    std::string receiverID;
    MessagingQos qosSettings;
    const bool isLocalMessage;
    Settings settings;
    ClusterControllerSettings ccSettings;
};

MATCHER_P(pointerToMqttAddressWithChannelId, channelId, "") {
    if (arg == nullptr) {
        return false;
    }
    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> mqttAddress
            = std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(arg);
    if (mqttAddress == nullptr) {
        return false;
    }
    return mqttAddress->getTopic() == channelId;
}

TEST_F(MqttMessagingSkeletonTest, transmitTest) {
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr, ccSettings.getMqttMulticastTopicPrefix());
    std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    EXPECT_CALL(*mockMessageRouter, route(immutableMessage,_)).Times(1);

    auto onFailure = [](const exceptions::JoynrRuntimeException& e) {
        FAIL() << "onFailure called";
    };
    mqttMessagingSkeleton.transmit(immutableMessage, onFailure);
}

void MqttMessagingSkeletonTest::transmitSetsIsReceivedFromGlobal()
{
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr, ccSettings.getMqttMulticastTopicPrefix());
    std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    EXPECT_FALSE(immutableMessage->isReceivedFromGlobal());
    auto onFailure = [](const exceptions::JoynrRuntimeException& e) {
        FAIL() << "onFailure called";
    };
    mqttMessagingSkeleton.transmit(immutableMessage, onFailure);
    EXPECT_TRUE(immutableMessage->isReceivedFromGlobal());
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsReceivedFromGlobalForMulticastPublications)
{
    MulticastPublication publication;
    mutableMessage = messageFactory.createMulticastPublication(
            senderID,
            qosSettings,
            publication
            );
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsReceivedFromGlobalForRequests)
{
    Request request;
    mutableMessage = messageFactory.createRequest(
            senderID,
            receiverID,
            qosSettings,
            request,
            isLocalMessage
            );
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsReceivedFromGlobalForSubscriptionRequests)
{
    SubscriptionRequest request;
    mutableMessage = messageFactory.createSubscriptionRequest(
            senderID,
            receiverID,
            qosSettings,
            request,
            isLocalMessage
            );
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsIsReceivedFromGlobalForBroadcastSubscriptionRequests)
{
    BroadcastSubscriptionRequest request;
    mutableMessage = messageFactory.createBroadcastSubscriptionRequest(
            senderID,
            receiverID,
            qosSettings,
            request,
            isLocalMessage
            );
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, transmitSetsIsReceivedFromGlobalForMulticastSubscriptionRequests)
{
    MulticastSubscriptionRequest request;
    mutableMessage = messageFactory.createMulticastSubscriptionRequest(
            senderID,
            receiverID,
            qosSettings,
            request,
            isLocalMessage
            );
    transmitSetsIsReceivedFromGlobal();
}

TEST_F(MqttMessagingSkeletonTest, onMessageReceivedTest) {
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, nullptr, ccSettings.getMqttMulticastTopicPrefix());
    std::unique_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockMessageRouter, route(
                    AllOf(
                        MessageHasType(mutableMessage.getType()),
                        ImmutableMessageHasPayload(mutableMessage.getPayload())
                        ),
                    _
                    )
                ).Times(1);

    smrf::ByteVector serializedMessage = immutableMessage->getSerializedMessage();
    mqttMessagingSkeleton.onMessageReceived(std::move(serializedMessage));
}

TEST_F(MqttMessagingSkeletonTest, registerMulticastSubscription_subscribesToMqttTopic)
{
    std::string multicastId = "multicastId";
    Settings settings;
    ClusterControllerSettings ccSettings(settings);
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings, ccSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver, "");
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId));
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, registerMulticastSubscription_subscribesToMqttTopicOnlyOnce)
{
    std::string multicastId = "multicastId";
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings, ccSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver, ccSettings.getMqttMulticastTopicPrefix());
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, unregisterMulticastSubscription_unsubscribesFromMqttTopic)
{
    std::string multicastId = "multicastId";
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings, ccSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver, ccSettings.getMqttMulticastTopicPrefix());
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);

    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId));
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
}

TEST_F(MqttMessagingSkeletonTest, unregisterMulticastSubscription_unsubscribesFromMqttTopicOnlyForUnsubscribeOfLastReceiver)
{
    std::string multicastId = "multicastId";
    MessagingSettings messagingSettings(settings);
    auto mockMqttReceiver = std::make_shared<MockMqttReceiver>(messagingSettings, ccSettings);
    MqttMessagingSkeleton mqttMessagingSkeleton(mockMessageRouter, mockMqttReceiver, ccSettings.getMqttMulticastTopicPrefix());
    EXPECT_CALL(*mockMqttReceiver, subscribeToTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
    mqttMessagingSkeleton.registerMulticastSubscription(multicastId);

    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId)).Times(0);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
    Mock::VerifyAndClearExpectations(mockMqttReceiver.get());

    EXPECT_CALL(*mockMqttReceiver, unsubscribeFromTopic(multicastId)).Times(1);
    mqttMessagingSkeleton.unregisterMulticastSubscription(multicastId);
}


TEST_F(MqttMessagingSkeletonTest, translateWildcard)
{
    std::map<std::string, std::string> input2expected = {
      {"partion0/partion1", "partion0/partion1"},
      {"partion0/partion1/*", "partion0/partion1/#"},
      {"partion0/partion1/+", "partion0/partion1/+"}
    };

    for(auto& testCase : input2expected) {
        std::string inputPartition = testCase.first;
        std::string expectedPartition = testCase.second;
        EXPECT_EQ(expectedPartition, 
                  MqttMessagingSkeleton::translateMulticastWildcard(inputPartition)
        );
    }
}

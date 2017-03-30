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
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStub.h"

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "tests/utils/MockObjects.h"

using namespace ::testing;
using namespace joynr;

class MqttMessagingStubTest : public ::testing::Test {
public:
    MqttMessagingStubTest() :
        destinationAddress(),
        mockMessageSender(std::make_shared<MockMessageSender>()),
        mqttMessagingStub(mockMessageSender, destinationAddress),
        messageFactory(),
        message(),
        qosSettings(456000)
    {
        std::string postFix = "_" + util::createUuid();
        senderID = "senderId" + postFix;
        receiverID = "receiverID" + postFix;
    }

protected:
    void transmitSetsHeaderReplyAddress();
    system::RoutingTypes::MqttAddress destinationAddress;
    std::string globalClusterControllerAddress;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    MqttMessagingStub mqttMessagingStub;
    JoynrMessageFactory messageFactory;
    JoynrMessage message;
    std::string replyAddressSerialized;
    std::string senderID;
    std::string receiverID;
    MessagingQos qosSettings;
};

void MqttMessagingStubTest::transmitSetsHeaderReplyAddress()
{
    JoynrMessage messageWithHeaderReplyAddress = message;
    messageWithHeaderReplyAddress.setHeaderReplyAddress(globalClusterControllerAddress);
    EXPECT_CALL(*mockMessageSender, sendMessage(
        Eq(destinationAddress),
        Eq(messageWithHeaderReplyAddress),
        _)
    ).Times(1);
    auto onFailure = [](const exceptions::JoynrRuntimeException& e) {
        FAIL() << "onFailure called";
    };
    EXPECT_TRUE(message.getHeaderReplyAddress().empty());
    mqttMessagingStub.transmit(message, onFailure);
    EXPECT_FALSE(message.getHeaderReplyAddress().empty());
    EXPECT_EQ(globalClusterControllerAddress, message.getHeaderReplyAddress());
}

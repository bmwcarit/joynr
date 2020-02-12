/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <joynr/BrokerUrl.h>
#include "joynr/ClusterControllerSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "libjoynrclustercontroller/mqtt/MqttSender.h"
#include "joynr/Settings.h"
#include "tests/mock/MockMosquittoConnection.h"

using namespace ::testing;

namespace joynr
{

class MqttSenderTest : public testing::Test
{
public:
    MqttSenderTest() : mqttAddress("brokerUri", "clientId"), mockMosquittoConnection(), mqttSender()
    {
    }

protected:
    void createMqttSender(std::string testSettingsFileNameMqtt)
    {
        Settings testSettings(testSettingsFileNameMqtt);
        MessagingSettings messagingSettings(testSettings);
        const ClusterControllerSettings ccSettings(testSettings);
        BrokerUrl brokerUrl("testBrokerUrl");
        std::chrono::seconds mqttKeepAliveTimeSeconds(1);
        std::chrono::seconds mqttReconnectDelayTimeSeconds(1);
        std::chrono::seconds mqttReconnectMaxDelayTimeSeconds(1);
        bool isMqttExponentialBackoffEnabled(false);
        const std::string clientId("testClientId");

        mockMosquittoConnection =
                std::make_shared<MockMosquittoConnection>(ccSettings,
                                                          brokerUrl,
                                                          mqttKeepAliveTimeSeconds,
                                                          mqttReconnectDelayTimeSeconds,
                                                          mqttReconnectMaxDelayTimeSeconds,
                                                          isMqttExponentialBackoffEnabled,
                                                          clientId);

        mqttSender = std::make_shared<MqttSender>(mockMosquittoConnection, messagingSettings);

        ON_CALL(*mockMosquittoConnection, isSubscribedToChannelTopic()).WillByDefault(Return(true));
        ON_CALL(*mockMosquittoConnection, getMqttQos()).WillByDefault(Return(0));
        ON_CALL(*mockMosquittoConnection, getMqttPrio()).WillByDefault(Return("low"));
    }

    ADD_LOGGER(MqttSenderTest)
    joynr::system::RoutingTypes::MqttAddress mqttAddress;

    std::shared_ptr<MockMosquittoConnection> mockMosquittoConnection;
    std::shared_ptr<MqttSender> mqttSender;
};

TEST_F(MqttSenderTest, messagePublishedToCorrectTopic)
{
    const std::string expectedTopic = mqttAddress.getTopic() + "/low";
    MutableMessage mutableMessage;

    createMqttSender("test-resources/MqttSenderTestWithMaxMessageSizeLimits2.settings");

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    mutableMessage.setSender("testSender");
    mutableMessage.setRecipient("testRecipient");
    mutableMessage.setPayload("shortMessage");

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockMosquittoConnection, publishMessage(
                    Eq(expectedTopic),
                    Eq(0),
                    _,
                    _,
                    _,
                    _));

    mqttSender->sendMessage(mqttAddress,
                            immutableMessage,
                            [] (const exceptions::JoynrRuntimeException& exception) {
                                FAIL() << "sendMessage failed: " << exception.getMessage();
                            });
}

TEST_F(MqttSenderTest, messagePublishedWithMsgTtlSecAlwaysRoundedUp)
{
    const TimePoint now = TimePoint::now();
    const std::uint32_t expectedRoundedMsgTtlSec = 61;
    auto onFailure = [] (const exceptions::JoynrRuntimeException& exception) {
        FAIL() << "sendMessage failed: " << exception.getMessage();
    };

    MutableMessage mutableMessage;

    createMqttSender("test-resources/MqttSenderTestWithMaxMessageSizeLimits2.settings");

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    mutableMessage.setSender("testSender");
    mutableMessage.setRecipient("testRecipient");
    mutableMessage.setPayload("shortMessage");
    mutableMessage.setExpiryDate(now + std::chrono::milliseconds(60700)); // 60,7 sec will be rounded to 61 sec

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage1 =
            mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockMosquittoConnection, publishMessage(
                    _,
                    _,
                    _,
                    Eq(expectedRoundedMsgTtlSec),
                    _,
                    _));

    mqttSender->sendMessage(mqttAddress, immutableMessage1, onFailure);

    auto relativeTtl1 = immutableMessage1->getExpiryDate().relativeFromNow().count();

    ASSERT_TRUE(relativeTtl1 % 1000 > 600 && relativeTtl1 % 1000 < 800);

    // Second message with different expiry date
    mutableMessage.setExpiryDate(now + std::chrono::milliseconds(60200)); // 60,2 sec will be rounded to 61 sec

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage2 =
            mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockMosquittoConnection, publishMessage(
                    _,
                    _,
                    _,
                    Eq(expectedRoundedMsgTtlSec),
                    _,
                    _));

    mqttSender->sendMessage(mqttAddress, immutableMessage2, onFailure);

    auto relativeTtl2 = immutableMessage2->getExpiryDate().relativeFromNow().count();

    ASSERT_TRUE(relativeTtl2 % 1000 > 100 && relativeTtl2 % 1000 < 300);
}

TEST_F(MqttSenderTest, messagePublishedWithMsgTtlSecGreaterThanMaxIntervalAlwaysSetToMaxInterval)
{
    const TimePoint now = TimePoint::now();
    const std::uint32_t MESSAGE_EXPIRY_MAX_INTERVAL = std::numeric_limits<std::uint32_t>::max();
    const std::uint32_t expectedMaxMsgTtlSec = MESSAGE_EXPIRY_MAX_INTERVAL;

    auto onFailure = [] (const exceptions::JoynrRuntimeException& exception) {
        FAIL() << "sendMessage failed: " << exception.getMessage();
    };

    MutableMessage mutableMessage;

    createMqttSender("test-resources/MqttSenderTestWithMaxMessageSizeLimits2.settings");

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    mutableMessage.setSender("testSender");
    mutableMessage.setRecipient("testRecipient");
    mutableMessage.setPayload("shortMessage");

    mutableMessage.setExpiryDate(now + std::chrono::seconds(MESSAGE_EXPIRY_MAX_INTERVAL) +
                                 std::chrono::seconds(6));

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage1 =
            mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockMosquittoConnection, publishMessage(
                    _,
                    _,
                    _,
                    Eq(expectedMaxMsgTtlSec),
                    _,
                    _)).Times(2);

    mqttSender->sendMessage(mqttAddress, immutableMessage1, onFailure);

    mutableMessage.setExpiryDate(TimePoint::fromAbsoluteMs(-1000));

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage2 =
            mutableMessage.getImmutableMessage();

    mqttSender->sendMessage(mqttAddress, immutableMessage2, onFailure);

}

TEST_F(MqttSenderTest, multicastMessagePublishedToCorrectTopic)
{
    const std::string expectedTopic = mqttAddress.getTopic();
    MutableMessage mutableMessage;

    createMqttSender("test-resources/MqttSenderTestWithMaxMessageSizeLimits2.settings");

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    mutableMessage.setSender("testSender");
    mutableMessage.setRecipient("testMulticastId");
    mutableMessage.setPayload("shortMessage");

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockMosquittoConnection, publishMessage(
                    Eq(expectedTopic),
                    Eq(0),
                    _,
                    _,
                    _,
                    _));

    mqttSender->sendMessage(mqttAddress,
                            immutableMessage,
                            [] (const exceptions::JoynrRuntimeException& exception) {
                                FAIL() << "sendMessage failed: " << exception.getMessage();
                            });
}

TEST_F(MqttSenderTest, TestWithEnabledMessageSizeCheck)
{
    MutableMessage mutableMessage;
    bool gotExpectedExceptionType;
    bool gotCalled;

    createMqttSender("test-resources/MqttSenderTestWithMaxMessageSizeLimits1.settings");

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    mutableMessage.setSender("testSender");
    mutableMessage.setRecipient("testMulticastId");
    mutableMessage.setPayload("shortMessage");

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            mutableMessage.getImmutableMessage();

    gotCalled = false;
    mqttSender->sendMessage(mqttAddress,
                            immutableMessage,
                            [&gotCalled, &gotExpectedExceptionType](
                                    const exceptions::JoynrRuntimeException& exception) {
        gotCalled = true;
        try {
            const joynr::exceptions::JoynrMessageNotSentException& messageNotSentException =
                    dynamic_cast<const joynr::exceptions::JoynrMessageNotSentException&>(exception);
            std::ignore = messageNotSentException;
        } catch (std::bad_cast& bc) {
            // fallthrough
        }
    });
    EXPECT_FALSE(gotCalled);

    std::string longMessagePayload(1000, 'x');
    mutableMessage.setPayload(longMessagePayload);
    immutableMessage = mutableMessage.getImmutableMessage();

    gotExpectedExceptionType = false;
    gotCalled = false;
    mqttSender->sendMessage(mqttAddress,
                            immutableMessage,
                            [&gotCalled, &gotExpectedExceptionType](
                                    const exceptions::JoynrRuntimeException& exception) {
        gotCalled = true;
        try {
            const joynr::exceptions::JoynrMessageNotSentException& messageNotSentException =
                    dynamic_cast<const joynr::exceptions::JoynrMessageNotSentException&>(exception);
            gotExpectedExceptionType = true;
            std::ignore = messageNotSentException;
        } catch (std::bad_cast& bc) {
            // fallthrough
        }
    });
    EXPECT_TRUE(gotCalled);
    EXPECT_TRUE(gotExpectedExceptionType);
}

TEST_F(MqttSenderTest, TestWithDisabledMessageSizeCheck)
{
    MutableMessage mutableMessage;

    createMqttSender("test-resources/MqttSenderTestWithMaxMessageSizeLimits2.settings");

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    mutableMessage.setSender("testSender");
    mutableMessage.setRecipient("testMulticastId");
    mutableMessage.setPayload("shortMessage");

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            mutableMessage.getImmutableMessage();

    bool gotCalled = false;
    mqttSender->sendMessage(
            mqttAddress,
            immutableMessage,
            [&gotCalled](const exceptions::JoynrRuntimeException& exception) { gotCalled = true; });
    EXPECT_FALSE(gotCalled);

    std::string longMessagePayload(1000, 'x');
    mutableMessage.setPayload(longMessagePayload);
    immutableMessage = mutableMessage.getImmutableMessage();

    gotCalled = false;
    mqttSender->sendMessage(
            mqttAddress,
            immutableMessage,
            [&gotCalled](const exceptions::JoynrRuntimeException& exception) { gotCalled = true; });
    EXPECT_FALSE(gotCalled);
}

} // namespace joynr

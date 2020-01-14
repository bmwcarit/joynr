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

#include "joynr/ClusterControllerSettings.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MutableMessage.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/TimePoint.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

#include "tests/JoynrTest.h"

using namespace joynr;

class MosquittoConnectionTest : public ::testing::Test
{
public:
    MosquittoConnectionTest()
    {
    }

    ~MosquittoConnectionTest() = default;

protected:
    std::shared_ptr<ImmutableMessage> createMessage(const TimePoint& expiryDate,
            const std::string& recipient,
            const std::string& payload = "")
    {
        MutableMessage mutableMsg;
        mutableMsg.setExpiryDate(expiryDate);
        mutableMsg.setRecipient(recipient);
        mutableMsg.setPayload(payload);
        return mutableMsg.getImmutableMessage();
    }

    static std::string payloadAsString(std::shared_ptr<ImmutableMessage> message)
    {
        const smrf::ByteArrayView& byteArrayView = message->getUnencryptedBody();
        return std::string(
                reinterpret_cast<const char*>(byteArrayView.data()), byteArrayView.size());
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnectionTest);
};

TEST_F(MosquittoConnectionTest, generalTest)
{
    Settings settings("test-resources/MqttJoynrClusterControllerRuntimeTest.settings");
    MessagingSettings messagingSettings(settings);
    ClusterControllerSettings ccSettings(settings);

    const BrokerUrl brokerUrl = messagingSettings.getBrokerUrl();
    std::chrono::seconds mqttKeepAliveTimeSeconds = messagingSettings.getMqttKeepAliveTimeSeconds();
    const std::chrono::seconds mqttReconnectDelayTimeSeconds = messagingSettings.getMqttReconnectDelayTimeSeconds();
    const std::chrono::seconds mqttReconnectMaxDelayTimeSeconds = messagingSettings.getMqttReconnectMaxDelayTimeSeconds();
    const bool isMqttExponentialBackoffEnabled = messagingSettings.getMqttExponentialBackoffEnabled();
    const std::string clientId1("clientId1");
    const std::string channelId1("channelId1");
    const std::string clientId2("clientId2");
    const std::string channelId2("channelId2");
    const std::string recipient2 = "recipient2";
    const std::string payload2 = "demoPayloadRequest";

    // create two MosquittoConnections using different clientIds and channels,
    // both connections connect to the same broker
    auto readyToSendSemaphore1 = std::make_shared<joynr::Semaphore>(0);
    auto mosquittoConnection1 = std::make_shared<MosquittoConnection>(
            ccSettings,
            brokerUrl,
            mqttKeepAliveTimeSeconds,
            mqttReconnectDelayTimeSeconds,
            mqttReconnectMaxDelayTimeSeconds,
            isMqttExponentialBackoffEnabled,
            clientId1);

    mosquittoConnection1->registerChannelId(channelId1);
    mosquittoConnection1->registerReceiveCallback([](smrf::ByteVector&& msg) {
            std::ignore = msg;
    });
    mosquittoConnection1->registerReadyToSendChangedCallback([readyToSendSemaphore1](bool readyToSend) {
            if (readyToSend) {
                readyToSendSemaphore1->notify();
            }
    });

    // after connection is established, subscription to channel based topic
    // should be established automatically
    mosquittoConnection1->start();
    EXPECT_TRUE(readyToSendSemaphore1->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection1->isReadyToSend());

    auto mosquittoConnection2 = std::make_shared<MosquittoConnection>(
            ccSettings,
            brokerUrl,
            mqttKeepAliveTimeSeconds,
            mqttReconnectDelayTimeSeconds,
            mqttReconnectMaxDelayTimeSeconds,
            isMqttExponentialBackoffEnabled,
            clientId2);

    auto msgReceived2 = std::make_shared<joynr::Semaphore>(0);
    auto readyToSendSemaphore2 = std::make_shared<joynr::Semaphore>(0);

    mosquittoConnection2->registerChannelId(channelId2);

    mosquittoConnection2->registerReceiveCallback([recipient2, payload2, msgReceived2](smrf::ByteVector&& msg) {
            auto immutableMessage = std::make_shared<ImmutableMessage>(msg);
            EXPECT_EQ(immutableMessage->getRecipient(), recipient2);
            EXPECT_EQ(MosquittoConnectionTest::payloadAsString(immutableMessage), payload2);
            msgReceived2->notify();
    });
    mosquittoConnection2->registerReadyToSendChangedCallback([readyToSendSemaphore2](bool readyToSend) {
            if (readyToSend) {
                readyToSendSemaphore2->notify();
            }
    });

    mosquittoConnection2->start();
    EXPECT_TRUE(readyToSendSemaphore2->waitFor(std::chrono::seconds(10)));
    EXPECT_TRUE(mosquittoConnection2->isReadyToSend());

    // create a message and send it via mosquittoConnection1 to mosquittoConnection2
    const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure = nullptr;
    const auto now = TimePoint::now();
    auto immutableMessage = createMessage(now + 10000, recipient2, payload2);
    const smrf::ByteVector& rawMessage = immutableMessage->getSerializedMessage();
    const std::string topic = channelId2 + "/low";
    const int qosLevel = mosquittoConnection1->getMqttQos();
    mosquittoConnection1->publishMessage(topic, qosLevel, onFailure, rawMessage.size(), rawMessage.data());

    // check the message has been received on mosquittoConnection2
    EXPECT_TRUE(msgReceived2->waitFor(std::chrono::seconds(10)));

    // subscribe and unsubscribe to/from an additional topic
    const std::string additionalTopic("additionalTopic");
    mosquittoConnection1->subscribeToTopic(additionalTopic);
    mosquittoConnection2->subscribeToTopic(additionalTopic);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    mosquittoConnection1->unsubscribeFromTopic(additionalTopic);
    mosquittoConnection2->unsubscribeFromTopic(additionalTopic);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // cleanup
    mosquittoConnection1->stop();
    mosquittoConnection2->stop();
}

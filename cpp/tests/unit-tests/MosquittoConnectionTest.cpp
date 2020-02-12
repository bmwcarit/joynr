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
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

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
    MosquittoConnectionTest() :
        _settings("test-resources/MqttJoynrClusterControllerRuntimeTest.settings"),
        _messagingSettings(_settings),
        _ccSettings(_settings),
        _brokerUrl(_messagingSettings.getBrokerUrl()),
        _mqttKeepAliveTimeSeconds(_messagingSettings.getMqttKeepAliveTimeSeconds()),
        _mqttReconnectDelayTimeSeconds(_messagingSettings.getMqttReconnectDelayTimeSeconds()),
        _mqttReconnectMaxDelayTimeSeconds(_messagingSettings.getMqttReconnectMaxDelayTimeSeconds()),
        _isMqttExponentialBackoffEnabled(_messagingSettings.getMqttExponentialBackoffEnabled())
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

    std::shared_ptr<MosquittoConnection> createMosquittoConnection(
            std::shared_ptr<joynr::Semaphore> readyToSendSemaphore,
            const std::string clientId,
            const std::string channelId
            )
    {
        auto mosquittoConnection = std::make_shared<MosquittoConnection>(
                _ccSettings,
                _brokerUrl,
                _mqttKeepAliveTimeSeconds,
                _mqttReconnectDelayTimeSeconds,
                _mqttReconnectMaxDelayTimeSeconds,
                _isMqttExponentialBackoffEnabled,
                clientId);

        // register connection to channelId
        mosquittoConnection->registerChannelId(channelId);
        // connection is established and ready to send messages
        mosquittoConnection->registerReadyToSendChangedCallback([readyToSendSemaphore](bool readyToSend) {
                if (readyToSend) {
                    readyToSendSemaphore->notify();
                }
        });
        return mosquittoConnection;
    }

    Settings _settings;
    MessagingSettings _messagingSettings;
    ClusterControllerSettings _ccSettings;
    const BrokerUrl _brokerUrl;
    std::chrono::seconds _mqttKeepAliveTimeSeconds;
    const std::chrono::seconds _mqttReconnectDelayTimeSeconds, _mqttReconnectMaxDelayTimeSeconds;
    const bool _isMqttExponentialBackoffEnabled;

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnectionTest);
};

TEST_F(MosquittoConnectionTest, generalTest)
{
    // These values remain persisted in the broker
    // use different ones when you need to create other connections
    const std::string clientId1("clientId1");
    const std::string clientId2("clientId2");
    const std::string channelId1("channelId1");
    const std::string channelId2("channelId2");

    // create two MosquittoConnections using different clientIds and channels,
    // both connections connect to the same broker
    auto readyToSendSemaphore1 = std::make_shared<joynr::Semaphore>(0);
    auto mosquittoConnection1 = createMosquittoConnection(
                readyToSendSemaphore1,
                clientId1,
                channelId1
                );
    mosquittoConnection1->registerReceiveCallback([](smrf::ByteVector&& msg) {
            std::ignore = msg;
    });

    // after connection is established, subscription to channel based topic
    // should be established automatically
    mosquittoConnection1->start();
    EXPECT_TRUE(readyToSendSemaphore1->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection1->isReadyToSend());

    auto readyToSendSemaphore2 = std::make_shared<joynr::Semaphore>(0);
    auto mosquittoConnection2 = createMosquittoConnection(
                readyToSendSemaphore2,
                clientId2,
                channelId2
                );

    auto msgReceived2 = std::make_shared<joynr::Semaphore>(0);
    const std::string recipient2 = "recipient2";
    const std::string payload2 = "payload2";

    mosquittoConnection2->registerReceiveCallback([recipient2, payload2, msgReceived2](smrf::ByteVector&& msg) {
            auto immutableMessage = std::make_shared<ImmutableMessage>(msg);
            EXPECT_EQ(immutableMessage->getRecipient(), recipient2);
            EXPECT_EQ(MosquittoConnectionTest::payloadAsString(immutableMessage), payload2);
            msgReceived2->notify();
    });

    mosquittoConnection2->start();
    EXPECT_TRUE(readyToSendSemaphore2->waitFor(std::chrono::seconds(10)));
    EXPECT_TRUE(mosquittoConnection2->isReadyToSend());

    // create a message and send it via mosquittoConnection1 to mosquittoConnection2
    const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure = nullptr;
    const auto now = TimePoint::now();
    auto immutableMessage = createMessage(now + 10000, recipient2, payload2);

    std::chrono::milliseconds msgTtlMs = immutableMessage->getExpiryDate().relativeFromNow();
    std::uint32_t msgTtlSec = static_cast<std::uint32_t>(std::ceil(msgTtlMs.count() / 1000.0));

    const smrf::ByteVector& rawMessage = immutableMessage->getSerializedMessage();
    const std::string topic = channelId2 + "/low";
    const int qosLevel = mosquittoConnection1->getMqttQos();
    mosquittoConnection1->publishMessage(topic,
                                         qosLevel,
                                         onFailure,
                                         msgTtlSec,
                                         rawMessage.size(),
                                         rawMessage.data());

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

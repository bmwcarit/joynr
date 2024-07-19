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
#include <ctime>
#include <memory>
#include <string>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

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
            : _settings("test-resources/MqttJoynrClusterControllerRuntimeTest.settings"),
              _messagingSettings(_settings),
              _ccSettings(_settings),
              _brokerUrl(_messagingSettings.getBrokerUrl()),
              _mqttKeepAliveTimeSeconds(_messagingSettings.getMqttKeepAliveTimeSeconds()),
              _mqttReconnectDelayTimeSeconds(_messagingSettings.getMqttReconnectDelayTimeSeconds()),
              _mqttReconnectMaxDelayTimeSeconds(
                      _messagingSettings.getMqttReconnectMaxDelayTimeSeconds()),
              _isMqttExponentialBackoffEnabled(
                      _messagingSettings.getMqttExponentialBackoffEnabled()),
              _isMqttRetain(_messagingSettings.getMqttRetain())
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
            const std::string channelId,
            const std::string gbid)
    {
        auto mosquittoConnection =
                std::make_shared<MosquittoConnection>(_ccSettings,
                                                      _brokerUrl,
                                                      _mqttKeepAliveTimeSeconds,
                                                      _mqttReconnectDelayTimeSeconds,
                                                      _mqttReconnectMaxDelayTimeSeconds,
                                                      _isMqttExponentialBackoffEnabled,
                                                      clientId,
                                                      gbid,
                                                      _isMqttRetain);

        // register connection to channelId
        mosquittoConnection->registerChannelId(channelId);
        // connection is established and ready to send messages
        mosquittoConnection->registerReadyToSendChangedCallback(
                [readyToSendSemaphore](bool readyToSend) {
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
    const bool _isMqttRetain;

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnectionTest);
};

TEST_F(MosquittoConnectionTest, generalTest)
{
    // These values remain persisted in the broker
    // use different ones when you need to create other connections
    std::srand(std::time(0));
    std::string random = std::to_string(std::rand());
    const std::string clientId1("clientId1-" + random);
    const std::string clientId2("clientId2-" + random);
    const std::string channelId1("channelId1-" + random);
    const std::string channelId2("channelId2-" + random);

    // create two MosquittoConnections using different clientIds and channels,
    // both connections connect to the same broker
    auto readyToSendSemaphore1 = std::make_shared<joynr::Semaphore>(0);
    std::string gbid1("gbid1");
    auto mosquittoConnection1 =
            createMosquittoConnection(readyToSendSemaphore1, clientId1, channelId1, gbid1);
    mosquittoConnection1->registerReceiveCallback([](smrf::ByteVector&& msg) {
        std::ignore = msg;
        ADD_FAILURE() << "We do not expect to receive msgs on connection1";
    });

    // after connection is established, subscription to channel based topic
    // should be established automatically
    mosquittoConnection1->start();
    EXPECT_TRUE(readyToSendSemaphore1->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection1->isReadyToSend());

    auto readyToSendSemaphore2 = std::make_shared<joynr::Semaphore>(0);
    std::string gbid2("gbid2");
    auto mosquittoConnection2 =
            createMosquittoConnection(readyToSendSemaphore2, clientId2, channelId2, gbid2);

    auto msgReceived2 = std::make_shared<joynr::Semaphore>(0);
    const std::string recipient2 = "recipient2";
    const std::string payload2 = "payload2";

    mosquittoConnection2->registerReceiveCallback(
            [recipient2, payload2, msgReceived2](smrf::ByteVector&& msg) {
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

    const std::uint32_t msgTtlSec = 60;
    const smrf::ByteVector& rawMessage = immutableMessage->getSerializedMessage();
    const std::string topic = channelId2 + "/low";
    const int qosLevel = mosquittoConnection1->getMqttQos();
    mosquittoConnection1->publishMessage(topic,
                                         qosLevel,
                                         onFailure,
                                         msgTtlSec,
                                         immutableMessage->getPrefixedCustomHeaders(),
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

TEST_F(MosquittoConnectionTest, deliverMessageWithinItsExpiryIntervalAfterReconnect)
{
    std::srand(std::time(0));
    std::string random = std::to_string(std::rand());
    const std::string clientId3("clientId3-" + random);
    const std::string clientId4("clientId4-" + random);
    const std::string channelId3("channelId3-" + random);
    const std::string channelId4("channelId4-" + random);

    // connection1
    auto readyToSendSemaphore1 = std::make_shared<joynr::Semaphore>(0);
    std::string gbid3("gbid3");
    auto mosquittoConnection1 =
            createMosquittoConnection(readyToSendSemaphore1, clientId3, channelId3, gbid3);

    // we do not plan to receive msgs on connection1, but we have to
    // set registerReceiveCallback.
    mosquittoConnection1->registerReceiveCallback(
            [](smrf::ByteVector&& msg) { std::ignore = msg; });

    mosquittoConnection1->start();
    EXPECT_TRUE(readyToSendSemaphore1->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection1->isReadyToSend());

    // connection2
    auto readyToSendSemaphore2 = std::make_shared<joynr::Semaphore>(0);
    std::string gbid4("gbid4");
    auto mosquittoConnection2 =
            createMosquittoConnection(readyToSendSemaphore2, clientId4, channelId4, gbid4);

    const std::string dummyRecipient = "dummyRecipient";
    const std::string dummyPayload = "dummyPayload";
    // check the message has been received on mosquittoConnection2
    auto msgWithExpiryDateReceived = std::make_shared<joynr::Semaphore>(0);
    mosquittoConnection2->registerReceiveCallback(
            [dummyRecipient, dummyPayload, msgWithExpiryDateReceived](smrf::ByteVector&& msg) {
                auto receivedImmutableMessage = std::make_shared<ImmutableMessage>(msg);
                EXPECT_EQ(receivedImmutableMessage->getRecipient(), dummyRecipient);
                EXPECT_EQ(MosquittoConnectionTest::payloadAsString(receivedImmutableMessage),
                          dummyPayload);
                msgWithExpiryDateReceived->notify();
            });

    mosquittoConnection2->start();
    EXPECT_TRUE(readyToSendSemaphore2->waitFor(std::chrono::seconds(10)));
    EXPECT_TRUE(mosquittoConnection2->isReadyToSend());

    mosquittoConnection2->stop();

    // prepare a message and publish it via the broker to connection2 (while it is unavailable)
    const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure = nullptr;
    const auto now = TimePoint::now();
    const std::int64_t msgDurationMs = 6000; // 1M
    auto immutableMessage = createMessage(now + msgDurationMs, dummyRecipient, dummyPayload);

    std::chrono::milliseconds msgTtlMs = immutableMessage->getExpiryDate().relativeFromNow();
    std::uint32_t msgTtlSec = static_cast<std::uint32_t>(std::ceil(msgTtlMs.count() / 1000.0));

    const smrf::ByteVector& rawMessage = immutableMessage->getSerializedMessage();
    const std::string topic = channelId4 + "/low";
    const int qosLevel = mosquittoConnection1->getMqttQos();

    mosquittoConnection1->publishMessage(topic,
                                         qosLevel,
                                         onFailure,
                                         msgTtlSec,
                                         immutableMessage->getPrefixedCustomHeaders(),
                                         rawMessage.size(),
                                         rawMessage.data());

    std::this_thread::sleep_for(std::chrono::seconds(2));
    // The broker should keep the msg for 1M
    // start connection2 before message expires at the broker
    mosquittoConnection2->start();
    EXPECT_TRUE(readyToSendSemaphore2->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection2->isReadyToSend());

    // message still not expired it will be received
    EXPECT_TRUE(msgWithExpiryDateReceived->waitFor(std::chrono::seconds(5)));

    // cleanup
    mosquittoConnection1->stop();
    mosquittoConnection2->stop();
}

TEST_F(MosquittoConnectionTest, noMessageDeliveryWhenExceedingItsExpiryIntervalAfterReconnect)
{
    std::srand(std::time(0));
    std::string random = std::to_string(std::rand());
    const std::string clientId5("clientId5-" + random);
    const std::string clientId6("clientId6-" + random);
    const std::string channelId5("channelId5-" + random);
    const std::string channelId6("channelId6-" + random);

    // connection1
    auto readyToSendSemaphore1 = std::make_shared<joynr::Semaphore>(0);
    std::string gbid5("gbid5");
    auto mosquittoConnection1 =
            createMosquittoConnection(readyToSendSemaphore1, clientId5, channelId5, gbid5);

    // we do not plan to receive msgs on connection1, but we have to
    // set registerReceiveCallback.
    mosquittoConnection1->registerReceiveCallback([](smrf::ByteVector&& msg) {
        std::ignore = msg;
        ADD_FAILURE() << "We do not expect to receive msgs on connection1";
    });

    mosquittoConnection1->start();
    EXPECT_TRUE(readyToSendSemaphore1->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection1->isReadyToSend());

    // connection2
    auto readyToSendSemaphore2 = std::make_shared<joynr::Semaphore>(0);
    std::string gbid6("gbid6");
    auto mosquittoConnection2 =
            createMosquittoConnection(readyToSendSemaphore2, clientId6, channelId6, gbid6);

    const std::string anotherDummyRecipient = "anotherDummyRecipient";
    const std::string anotherDummyPayload = "anotherDummyPayload";
    // check the message has been received on mosquittoConnection2
    auto msgWithExpiryDateReceived = std::make_shared<joynr::Semaphore>(0);
    mosquittoConnection2->registerReceiveCallback(
            [msgWithExpiryDateReceived](smrf::ByteVector&& msg) {
                std::ignore = msg;
                msgWithExpiryDateReceived->notify();
            });

    mosquittoConnection2->start();
    EXPECT_TRUE(readyToSendSemaphore2->waitFor(std::chrono::seconds(10)));
    EXPECT_TRUE(mosquittoConnection2->isReadyToSend());

    mosquittoConnection2->stop();

    // prepare a message and publish it via the broker to connection2 (while it is unavailable)
    const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure = nullptr;
    const auto now = TimePoint::now();
    const std::int64_t msgDurationMs = 3000; // 3sec
    auto immutableMessage =
            createMessage(now + msgDurationMs, anotherDummyRecipient, anotherDummyPayload);

    std::chrono::milliseconds msgTtlMs = immutableMessage->getExpiryDate().relativeFromNow();
    std::uint32_t msgTtlSec = static_cast<std::uint32_t>(std::ceil(msgTtlMs.count() / 1000.0));

    const smrf::ByteVector& rawMessage = immutableMessage->getSerializedMessage();
    const std::string topic = channelId6 + "/low";
    const int qosLevel = mosquittoConnection1->getMqttQos();

    mosquittoConnection1->publishMessage(topic,
                                         qosLevel,
                                         onFailure,
                                         msgTtlSec,
                                         immutableMessage->getPrefixedCustomHeaders(),
                                         rawMessage.size(),
                                         rawMessage.data());

    // The broker should keep the msg for 3sec
    std::this_thread::sleep_for(std::chrono::seconds(4));
    // start connection2 after message expired at the broker
    mosquittoConnection2->start();
    EXPECT_TRUE(readyToSendSemaphore2->waitFor(std::chrono::seconds(5)));
    EXPECT_TRUE(mosquittoConnection2->isReadyToSend());

    // message expired nothing will be received
    EXPECT_FALSE(msgWithExpiryDateReceived->waitFor(std::chrono::seconds(3)));

    // cleanup
    mosquittoConnection1->stop();
    mosquittoConnection2->stop();
}

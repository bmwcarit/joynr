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
#ifndef CLUSTER_CONTROLLER_MQTT_MOSQUITTOSUBSCRIBER_H_
#define CLUSTER_CONTROLLER_MQTT_MOSQUITTOSUBSCRIBER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "mosquittopp.h"

#include "MosquittoConnection.h"
#include "MqttSettings.h"

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Thread.h"

namespace joynr
{

class JoynrMessage;
class MessageRouter;
class MessagingSettings;

class MosquittoSubscriber : public Thread, MosquittoConnection
{
public:
    explicit MosquittoSubscriber(const MessagingSettings& settings,
                                 const std::string& channelId,
                                 Semaphore* channelCreatedSemaphore);

    ~MosquittoSubscriber() override = default;

    void stop() override;
    void run() override;

    void registerChannelId(const std::string& channelId);
    void registerReceiveCallback(std::function<void(const std::string&)> onTextMessageReceived);

    void interrupt();
    bool isInterrupted();

    /* subscribe to channelId / topic */
    void subscribeToTopic(const std::string& topic);

    /* unsubscribe from channelId / topic */
    void unsubscribeFromTopic(const std::string& topic);

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoSubscriber);

    MqttSettings mqttSettings;
    std::string channelId;
    std::string topic;
    std::unordered_set<std::string> additionalTopics;
    std::recursive_mutex additionalTopicsMutex;
    Semaphore* channelCreatedSemaphore;

    std::atomic<bool> isConnected;
    std::atomic<bool> isRunning;
    std::atomic<bool> isChannelAvailable;

    /*! On text message received callback */
    std::function<void(const std::string&)> onTextMessageReceived;

    ADD_LOGGER(MqttSubscriber);

    void checkServerTime();

    void restoreSubscriptions();
    void subscribeToTopicInternal(const std::string& topic);

    void on_connect(int rc) override;
    void on_subscribe(int mid, int qos_count, const int* granted_qos) override;
    void on_message(const struct mosquitto_message* message) override;
};

} // namespace joynr

#endif // CLUSTER_CONTROLLER_MQTT_MOSQUITTOSUBSCRIBER_H_

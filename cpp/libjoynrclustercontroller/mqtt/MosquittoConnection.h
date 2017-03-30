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
#ifndef MOSQUITTOCONNECTION_H
#define MOSQUITTOCONNECTION_H

#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <unordered_set>

#include <mosquittopp.h>

#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class MessagingSettings;
class ClusterControllerSettings;

class MosquittoConnection : public mosqpp::mosquittopp
{

public:
    explicit MosquittoConnection(const MessagingSettings& messagingSettings,
                                 const ClusterControllerSettings& ccSettings,
                                 const std::string& clientId);

    ~MosquittoConnection() override;

    virtual std::uint16_t getMqttQos() const;
    std::string getMqttPrio() const;
    bool isMqttRetain() const;

    void start();
    void stop();

    void publishMessage(
            const std::string& topic,
            const int qosLevel,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
            std::uint32_t payloadlen,
            const void* payload);
    void subscribeToTopic(const std::string& topic);
    void unsubscribeFromTopic(const std::string& topic);
    void registerChannelId(const std::string& channelId);
    void registerReceiveCallback(std::function<void(const std::string&)> onTextMessageReceived);
    bool isSubscribedToChannelTopic() const;

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnection);

    void runLoop();

    void on_disconnect(int rc) final;
    void on_log(int level, const char* str) final;
    void on_error() final;
    void on_connect(int rc) final;
    void on_message(const mosquitto_message* message) final;
    void on_publish(int mid) final;
    void on_subscribe(int mid, int qos_count, const int* granted_qos) final;
    void restoreSubscriptions();
    void subscribeToTopicInternal(const std::string& topic, const bool isChannelTopic = false);

    const MessagingSettings& messagingSettings;
    const std::string host;
    const std::uint16_t port;

    const std::uint16_t mqttQos = 1;
    const bool mqttRetain = false;

    std::string channelId;
    int subscribeChannelMid;
    std::string topic;
    std::unordered_set<std::string> additionalTopics;
    std::recursive_mutex additionalTopicsMutex;

    std::atomic<bool> isConnected;
    std::atomic<bool> isRunning;
    std::atomic<bool> isChannelIdRegistered;
    std::atomic<bool> subscribedToChannelTopic;

    std::function<void(const std::string&)> onTextMessageReceived;

    std::thread thread;

    ADD_LOGGER(MosquittoConnection);
};

} // namespace joynr

#endif // MOSQUITTOCONNECTION_H

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
#ifndef MOSQUITTOCONNECTION_H
#define MOSQUITTOCONNECTION_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <mosquitto.h>
#pragma GCC diagnostic pop
#include <mqtt_protocol.h>

#if (LIBMOSQUITTO_VERSION_NUMBER < 1006007)
#error unsupported libmosquitto version, must be 1.6.7 or later
#endif

#include <smrf/ByteVector.h>

#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"

namespace joynr
{

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class ClusterControllerSettings;

class MosquittoConnection
{

public:
    explicit MosquittoConnection(const ClusterControllerSettings& ccSettings,
                                 BrokerUrl brokerUrl,
                                 std::chrono::seconds mqttKeepAliveTimeSeconds,
                                 std::chrono::seconds mqttReconnectDelayTimeSeconds,
                                 std::chrono::seconds mqttReconnectMaxDelayTimeSeconds,
                                 bool isMqttExponentialBackoffEnabled,
                                 const std::string& clientId,
                                 const std::string& gbid,
                                 bool isMqttRetain);

    virtual ~MosquittoConnection();

    virtual std::uint16_t getMqttQos() const;
    virtual std::string getMqttPrio() const;
    virtual std::uint32_t getMqttMaximumPacketSize() const;
    virtual bool isMqttRetain() const;

    /**
     * Starts mosquitto's internal loop in case it is not running or handles reconnect when
     * external communication with MQTT broker needs to be restored.
     */
    virtual void start();

    /**
     * Stops external communication by disconnecting from MQTT broker. Mosquitto loop is
     * not stopped.
     */
    virtual void stop();

    virtual void publishMessage(
            const std::string& topic,
            const int qosLevel,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
            const uint32_t msgTtlSec,
            const std::unordered_map<std::string, std::string> prefixedCustomHeaders,
            const uint32_t payloadlen,
            const void* payload);
    virtual void subscribeToTopic(const std::string& topic);
    virtual void unsubscribeFromTopic(const std::string& topic);
    virtual void registerChannelId(const std::string& channelId);
    virtual void registerReceiveCallback(std::function<void(smrf::ByteVector&&)> onMessageReceived);
    virtual void registerReadyToSendChangedCallback(std::function<void(bool)> readyToSendCallback);
    virtual bool isSubscribedToChannelTopic() const;
    virtual bool isReadyToSend() const;

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnection);

    static void on_connect_v5(struct mosquitto* mosq,
                              void* userdata,
                              int rc,
                              int flags,
                              const mosquitto_property* props);
    static void on_disconnect_v5(struct mosquitto* mosq,
                                 void* userdata,
                                 int rc,
                                 const mosquitto_property* props);
    static void on_publish_v5(struct mosquitto* mosq,
                              void* userdata,
                              int mid,
                              int reason_code,
                              const mosquitto_property* props);
    static void on_message_v5(struct mosquitto* mosq,
                              void* userdata,
                              const struct mosquitto_message* message,
                              const mosquitto_property* props);
    static void on_subscribe_v5(struct mosquitto* mosq,
                                void* userdata,
                                int mid,
                                int qos_count,
                                const int* granted_qos,
                                const mosquitto_property* props);
    static void on_log(struct mosquitto* mosq, void* userdata, int level, const char* str);

    void startInternal();
    void stopInternal();

    /**
     * Starts mosquitto's internal loop. This function wraps internal mosquitto function loop_start
     */
    void startLoop();

    /**
     * Stops mosquitto's internal loop. This function wraps internal mosquitto function loop_stop
     * @param force Set to true to force stopping of the loop. If false, stop (disconnect) must have
     * already been called.
     */
    void stopLoop(bool force = false);

    void cleanupLibrary();

    void createSubscriptions();
    void subscribeToTopicInternal(const std::string& _topic, const bool isChannelTopic = false);
    void setReadyToSend(bool readyToSend);
    static std::string getErrorString(int rc);

    const BrokerUrl _brokerUrl;
    const std::chrono::seconds _mqttKeepAliveTimeSeconds;
    const std::chrono::seconds _mqttReconnectDelayTimeSeconds;
    const std::chrono::seconds _mqttReconnectMaxDelayTimeSeconds;
    const bool _isMqttExponentialBackoffEnabled;
    const std::string _host;
    const std::uint16_t _port;

    const std::uint16_t _mqttQos = 1;
    const bool _mqttRetain;

    std::string _channelId;
    int _subscribeChannelMid;
    std::string _topic;
    std::unordered_set<std::string> _additionalTopics;
    std::recursive_mutex _additionalTopicsMutex;

    std::atomic<bool> _isConnected;
    std::atomic<bool> _isRunning;
    std::atomic<bool> _isChannelIdRegistered;
    std::atomic<bool> _subscribedToChannelTopic;
    std::atomic<bool> _readyToSend;
    static std::mutex _libUseCountMutex;
    static int _libUseCount;

    std::function<void(smrf::ByteVector&&)> _onMessageReceived;
    std::mutex _onReadyToSendChangedMutex;
    std::function<void(bool)> _onReadyToSendChanged;

    std::mutex _stopMutex;
    std::mutex _stopStartMutex;
    bool _isStopped;
    bool _isActive;
    std::atomic<bool> _isStopping;
    std::atomic<bool> _restartThreadShutdown;
    Semaphore _restartSemaphore;
    std::thread _restartThread;
    struct mosquitto* _mosq;
    uint32_t _mqttMaximumPacketSize;
    std::string _gbid;

    static constexpr std::int32_t sessionExpiryInterval = std::numeric_limits<std::int32_t>::max();

    ADD_LOGGER(MosquittoConnection)
};

} // namespace joynr

#endif // MOSQUITTOCONNECTION_H

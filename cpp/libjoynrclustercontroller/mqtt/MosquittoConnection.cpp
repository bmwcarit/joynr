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

#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

#include <openssl/ssl.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

MosquittoConnection::MosquittoConnection(const MessagingSettings& messagingSettings,
                                         const ClusterControllerSettings& ccSettings,
                                         const std::string& clientId)
        : mosquittopp(clientId.c_str(), false),
          messagingSettings(messagingSettings),
          host(messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getHost()),
          port(messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getPort()),
          channelId(),
          subscribeChannelMid(),
          topic(),
          additionalTopics(),
          additionalTopicsMutex(),
          isConnected(false),
          isRunning(false),
          isChannelIdRegistered(false),
          subscribedToChannelTopic(false),
          readyToSend(false),
          onMessageReceived(),
          onReadyToSendChangedMutex(),
          onReadyToSendChanged(),
          stopMutex(),
          stopStartMutex(),
          isStopped(true),
          isActive(false),
          restartThreadShutdown(false),
          restartSemaphore(0),
          restartThread()
{
    JOYNR_LOG_INFO(logger(), "Init mosquitto connection using MQTT client ID: {}", clientId);
    mosqpp::lib_init();

    if (ccSettings.isMqttUsernameSet()) {
        const std::string mqttUsername = ccSettings.getMqttUsername();
        const char* mqttUsername_cstr = mqttUsername.empty() ? nullptr : mqttUsername.c_str();
        const std::string mqttPassword = ccSettings.getMqttPassword();
        const char* mqttPassword_cstr = mqttPassword.empty() ? nullptr : mqttPassword.c_str();
        int rc = username_pw_set(mqttUsername_cstr, mqttPassword_cstr);
        if (rc != MOSQ_ERR_SUCCESS) {
            const std::string errorString(getErrorString(rc));
            JOYNR_LOG_ERROR(logger(),
                            "unable to set username/password for MQTT connection - {}",
                            errorString);
            return;
        }
    }

    if (ccSettings.isMqttTlsEnabled()) {
        const std::string mqttCertificateAuthorityPemFilename =
                ccSettings.getMqttCertificateAuthorityPemFilename();
        const std::string mqttCertificateAuthorityCertificateFolderPath =
                ccSettings.getMqttCertificateAuthorityCertificateFolderPath();
        const std::string mqttCertificatePemFilename = ccSettings.getMqttCertificatePemFilename();
        const std::string mqttPrivateKeyPemFilename = ccSettings.getMqttPrivateKeyPemFilename();

        const char* mqttCertificateAuthorityPemFilename_cstr =
                ccSettings.isMqttCertificateAuthorityPemFilenameSet()
                        ? mqttCertificateAuthorityPemFilename.c_str()
                        : nullptr;

        const char* mqttCertificateAuthorityCertificateFolderPath_cstr =
                ccSettings.isMqttCertificateAuthorityCertificateFolderPathSet()
                        ? mqttCertificateAuthorityCertificateFolderPath.c_str()
                        : nullptr;

        const char* mqttCertificatePemFilename_cstr = ccSettings.isMqttCertificatePemFilenameSet()
                                                              ? mqttCertificatePemFilename.c_str()
                                                              : nullptr;

        const char* mqttPrivateKeyPemFilename_cstr = ccSettings.isMqttPrivateKeyPemFilenameSet()
                                                             ? mqttPrivateKeyPemFilename.c_str()
                                                             : nullptr;

        int rc = tls_set(mqttCertificateAuthorityPemFilename_cstr,
                         mqttCertificateAuthorityCertificateFolderPath_cstr,
                         mqttCertificatePemFilename_cstr,
                         mqttPrivateKeyPemFilename_cstr);

        if (rc != MOSQ_ERR_SUCCESS) {
            mosqpp::lib_cleanup();
            const std::string message = "Mqtt TLS enabled, but TLS certificates are incorrectly "
                                        "specified or inaccessible: " +
                                        getErrorString(rc);
            JOYNR_LOG_FATAL(logger(), message);
            throw joynr::exceptions::JoynrRuntimeException(message);
        }

        const std::string tlsCiphers = ccSettings.getMqttTlsCiphers();
        rc = tls_opts_set(SSL_VERIFY_PEER,
                          ccSettings.getMqttTlsVersion().c_str(),
                          tlsCiphers.empty() ? nullptr : tlsCiphers.c_str());

        if (rc != MOSQ_ERR_SUCCESS) {
            mosqpp::lib_cleanup();
            const std::string message =
                    "fatal failure to initialize TLS connection, error settings TLS options: " +
                    getErrorString(rc);
            JOYNR_LOG_FATAL(logger(), message);
            throw joynr::exceptions::JoynrRuntimeException(message);
        }
    } else {
        JOYNR_LOG_DEBUG(logger(), "MQTT connection not encrypted");
    }

    restartThread = std::thread([this]() {
        for (;;) {
            if (restartThreadShutdown) {
                break;
            }

            JOYNR_LOG_INFO(logger(), "restartThread: waiting for restartSemaphore");
            restartSemaphore.wait();
            JOYNR_LOG_INFO(logger(), "restartThread: got notified by restartSemaphore");

            if (restartThreadShutdown) {
                break;
            }

            {
                std::lock_guard<std::mutex> stopStartLocker(stopStartMutex);
                if (isActive) {
                    JOYNR_LOG_INFO(logger(), "restartThread: calling stopInternal()");
                    stopInternal();
                } else {
                    JOYNR_LOG_INFO(
                            logger(), "restartThread: external comms disabled - skipping stop");
                    continue;
                }
            }

            if (restartThreadShutdown) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(
                    this->messagingSettings.getMqttReconnectDelayTimeSeconds().count()));

            if (restartThreadShutdown) {
                break;
            }

            {
                std::lock_guard<std::mutex> stopStartLocker(stopStartMutex);
                if (isActive) {
                    JOYNR_LOG_INFO(logger(), "restartThread: calling startInternal()");
                    startInternal();
                } else {
                    JOYNR_LOG_INFO(
                            logger(), "restartThread: external comms disabled - skipping restart");
                }
            }
        }
        JOYNR_LOG_INFO(logger(), "restartThread terminating");
    });
}

MosquittoConnection::~MosquittoConnection()
{
    std::lock_guard<std::mutex> stopLocker(stopMutex);
    assert(isStopped);
    assert(!isActive);

    // signal restartThread, we are about to shutdown
    restartThreadShutdown = true;
    restartSemaphore.notify();
    if (restartThread.joinable()) {
        JOYNR_LOG_INFO(logger(), "restartThread joinable");
        restartThread.join();
        JOYNR_LOG_INFO(logger(), "restartThread joined");
    } else {
        JOYNR_LOG_ERROR(logger(), "restartThread not joinable");
    }

    mosqpp::lib_cleanup();
}

std::string MosquittoConnection::getErrorString(int rc)
{
    // Do not use mosq::strerror() in case of MOSQ_ERR_ERRNO
    // since it calls the MT-unsafe API strerror()
    if (rc != MOSQ_ERR_ERRNO) {
        return std::string(mosqpp::strerror(rc));
    }

    const int storedErrno = errno;
    JOYNR_LOG_DEBUG(logger(), "getErrorString: MOSQ_ERR_ERRNO with errno = %d", storedErrno);
    return joynr::util::getErrorString(storedErrno);
}

void MosquittoConnection::on_disconnect(int rc)
{
    const int storedErrno = errno;
    const std::string errorString(getErrorString(rc));
    setReadyToSend(false);
    if (!isConnected) {
        // In case we didn't connect yet
        JOYNR_LOG_ERROR(logger(),
                        "Not yet connected to tcp://{}:{}, rc: {}, errno: {}, error: {}",
                        host,
                        port,
                        rc,
                        errno,
                        errorString);
        return;
    }

    // There was indeed a disconnect...set connect to false
    isConnected = false;

    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(), "Disconnected from tcp://{}:{}", host, port);
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Unexpectedly disconnected from tcp://{}:{}, rc: {}, errno: {}, error: {}",
                        host,
                        port,
                        rc,
                        storedErrno,
                        errorString);
        // trigger restart for all known fatal cases where Mosquitto leaves the loop
        // and thus does not attempt the reconnect itself
        switch (rc) {
        case MOSQ_ERR_NOMEM:
        case MOSQ_ERR_PROTOCOL:
        case MOSQ_ERR_INVAL:
        case MOSQ_ERR_NOT_FOUND:
        case MOSQ_ERR_PAYLOAD_SIZE:
        case MOSQ_ERR_NOT_SUPPORTED:
        case MOSQ_ERR_AUTH:
        case MOSQ_ERR_ACL_DENIED:
        case MOSQ_ERR_UNKNOWN:
        case MOSQ_ERR_EAI:
        case MOSQ_ERR_PROXY:
            JOYNR_LOG_INFO(logger(), "Triggering loop restart");
            restartSemaphore.notify();
            return;
        case MOSQ_ERR_ERRNO:
            if (storedErrno == EPROTO) {
                JOYNR_LOG_INFO(logger(), "Triggering loop restart");
                restartSemaphore.notify();
                return;
            }
            break;
        case MOSQ_ERR_CONN_PENDING:
        case MOSQ_ERR_NO_CONN:
        case MOSQ_ERR_CONN_REFUSED:
        case MOSQ_ERR_CONN_LOST:
        case MOSQ_ERR_TLS:
        default:
            break;
        }
        reconnect();
    }
}

void MosquittoConnection::on_log(int level, const char* str)
{
    if (level == MOSQ_LOG_ERR) {
        JOYNR_LOG_ERROR(logger(), "Mosquitto Log: {}", str);
    } else if (level == MOSQ_LOG_WARNING) {
        JOYNR_LOG_WARN(logger(), "Mosquitto Log: {}", str);
    } else if (level == MOSQ_LOG_INFO) {
        JOYNR_LOG_INFO(logger(), "Mosquitto Log: {}", str);
    } else {
        // MOSQ_LOG_NOTICE || MOSQ_LOG_DEBUG || any other log level
        JOYNR_LOG_DEBUG(logger(), "Mosquitto Log: {}", str);
    }
}

std::uint16_t MosquittoConnection::getMqttQos() const
{
    return mqttQos;
}

std::string MosquittoConnection::getMqttPrio() const
{
    static const std::string value("low");
    return value;
}

bool MosquittoConnection::isMqttRetain() const
{
    return mqttRetain;
}

void MosquittoConnection::start()
{
    JOYNR_LOG_INFO(logger(), "MosquittoConnection external start() called");
    std::lock_guard<std::mutex> stopStartLocker(stopStartMutex);
    startInternal();
    isActive = true;
}

void MosquittoConnection::startInternal()
{
    // do not start/stop in parallel
    std::lock_guard<std::mutex> stopLocker(stopMutex);
    if (!isStopped) {
        JOYNR_LOG_INFO(logger(), "Mosquitto Connection already started");
        return;
    }

    JOYNR_LOG_TRACE(
            logger(), "Start called with isRunning: {}, isConnected: {}", isRunning, isConnected);

    JOYNR_LOG_INFO(logger(), "Try to connect to tcp://{}:{}", host, port);

    connect_async(host.c_str(), port, messagingSettings.getMqttKeepAliveTimeSeconds().count());

    reconnect_delay_set(messagingSettings.getMqttReconnectDelayTimeSeconds().count(),
                        messagingSettings.getMqttReconnectMaxDelayTimeSeconds().count(),
                        messagingSettings.getMqttExponentialBackoffEnabled());

    startLoop();

    if (isRunning) {
        isStopped = false;
    }
}

void MosquittoConnection::startLoop()
{
    int rc = loop_start();
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(), "Mosquitto loop started");
        isRunning = true;
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "Mosquitto loop start failed: error: {} ({})",
                        std::to_string(rc),
                        errorString);
    }
}

void MosquittoConnection::stop()
{
    JOYNR_LOG_INFO(logger(), "MosquittoConnection external stop() called");
    std::lock_guard<std::mutex> stopStartLocker(stopStartMutex);
    stopInternal();
    isActive = false;
}

void MosquittoConnection::stopInternal()
{
    // do not start/stop in parallel
    std::lock_guard<std::mutex> stopLocker(stopMutex);
    if (isStopped) {
        JOYNR_LOG_INFO(logger(), "Mosquitto Connection already stopped");
        return;
    }

    // disconnect() must be called prior to stopLoop() since
    // otherwise the loop in the mosquitto background thread
    // continues forever and thus the join in stopLoop()
    // blocks indefinitely. This applies even if a connection
    // does not exist yet.

    if (isConnected) {
        int rc = disconnect();

        if (rc == MOSQ_ERR_SUCCESS) {
            JOYNR_LOG_INFO(logger(), "Mosquitto Connection disconnected");
        } else {
            const std::string errorString(getErrorString(rc));
            JOYNR_LOG_ERROR(logger(),
                            "Mosquitto disconnect failed: error: {} ({})",
                            std::to_string(rc),
                            errorString);
        }
        stopLoop();
    } else if (isRunning) {
        disconnect();
        stopLoop();
    }
    setReadyToSend(false);
    isStopped = true;
}

void MosquittoConnection::stopLoop(bool force)
{
    int rc = loop_stop(force);

    if (rc == MOSQ_ERR_SUCCESS) {
        isRunning = false;
        JOYNR_LOG_INFO(logger(), "Mosquitto loop stopped");
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "Mosquitto loop stop failed: error: {} ({})",
                        std::to_string(rc),
                        errorString);
    }
}

void MosquittoConnection::on_connect(int rc)
{
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(), "Mosquitto Connection established");
        isConnected = true;

        createSubscriptions();
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(
                logger(), "Mosquitto Connection Error: {} ({})", std::to_string(rc), errorString);
    }
}

void MosquittoConnection::createSubscriptions()
{
    while (!isChannelIdRegistered && isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    try {
        subscribeToTopicInternal(topic, true);
        std::lock_guard<std::recursive_mutex> lock(additionalTopicsMutex);
        for (const std::string& additionalTopic : additionalTopics) {
            subscribeToTopicInternal(additionalTopic);
        }
    } catch (const exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger(), "Error subscribing to Mqtt topic, error: ", error.getMessage());
    }
}

void MosquittoConnection::subscribeToTopicInternal(const std::string& topic,
                                                   const bool isChannelTopic)
{
    int* mid = nullptr;
    if (isChannelTopic) {
        mid = &subscribeChannelMid;
    }
    int rc = subscribe(mid, topic.c_str(), getMqttQos());
    switch (rc) {
    case (MOSQ_ERR_SUCCESS):
        JOYNR_LOG_INFO(logger(), "Subscribed to {}", topic);
        break;
    case (MOSQ_ERR_NO_CONN): {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_DEBUG(logger(),
                        "Subscription to {} failed: error: {} ({}). "
                        "Subscription will be restored on connect.",
                        topic,
                        std::to_string(rc),
                        errorString);
        break;
    }
    default: {
        // MOSQ_ERR_INVAL, MOSQ_ERR_NOMEM
        const std::string errorString(getErrorString(rc));
        std::string errorMsg = "Subscription to " + topic + " failed: error: " +
                               std::to_string(rc) + " (" + errorString + ")";
        throw exceptions::JoynrRuntimeException(errorMsg);
    }
    }
}

void MosquittoConnection::subscribeToTopic(const std::string& topic)
{
    if (!isChannelIdRegistered) {
        std::string errorMsg = "No channelId registered, cannot subscribe to topic " + topic;
        throw exceptions::JoynrRuntimeException(errorMsg);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(additionalTopicsMutex);
        if (additionalTopics.find(topic) != additionalTopics.end()) {
            JOYNR_LOG_DEBUG(logger(), "Already subscribed to topic {}", topic);
            return;
        }

        subscribeToTopicInternal(topic);

        additionalTopics.insert(topic);
    }
}

void MosquittoConnection::unsubscribeFromTopic(const std::string& topic)
{
    if (isChannelIdRegistered) {
        std::lock_guard<std::recursive_mutex> lock(additionalTopicsMutex);
        if (additionalTopics.find(topic) == additionalTopics.end()) {
            JOYNR_LOG_DEBUG(logger(), "Unsubscribe called for non existing topic {}", topic);
            return;
        }
        additionalTopics.erase(topic);
        if (isConnected && isRunning) {
            int rc = unsubscribe(nullptr, topic.c_str());
            if (rc == MOSQ_ERR_SUCCESS) {
                JOYNR_LOG_INFO(logger(), "Unsubscribed from {}", topic);
            } else {
                // MOSQ_ERR_INVAL || MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN
                const std::string errorString(getErrorString(rc));
                JOYNR_LOG_ERROR(logger(),
                                "Unsubscribe from {} failed: error: {} ({})",
                                topic,
                                std::to_string(rc),
                                errorString);
            }
        }
    }
}

void MosquittoConnection::publishMessage(
        const std::string& topic,
        const int qosLevel,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
        uint32_t payloadlen = 0,
        const void* payload = nullptr)
{
    JOYNR_LOG_DEBUG(logger(), "Publish message of length {} to {}", payloadlen, topic);

    int mid;
    int rc = publish(&mid, topic.c_str(), payloadlen, payload, qosLevel, isMqttRetain());
    if (!(rc == MOSQ_ERR_SUCCESS)) {
        const std::string errorString(getErrorString(rc));
        if (rc == MOSQ_ERR_INVAL || rc == MOSQ_ERR_PAYLOAD_SIZE) {
            onFailure(exceptions::JoynrMessageNotSentException(
                    "message could not be sent: mid (mqtt message id): " + std::to_string(mid) +
                    ", error: " + std::to_string(rc) + " (" + errorString + ")"));
            return;
        }
        // MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN || MOSQ_ERR_PROTOCOL ||| unexpected errors
        onFailure(exceptions::JoynrDelayMessageException(
                "error sending message: mid (mqtt message id): " + std::to_string(mid) +
                ", error: " + std::to_string(rc) + " (" + errorString + ")"));
        return;
    }
    JOYNR_LOG_TRACE(logger(), "published message with mqtt message id {}", std::to_string(mid));
}

void MosquittoConnection::registerChannelId(const std::string& channelId)
{
    this->channelId = channelId;
    topic = channelId + "/" + getMqttPrio() + "/" + "#";
    isChannelIdRegistered = true;
}

void MosquittoConnection::registerReceiveCallback(
        std::function<void(smrf::ByteVector&&)> onMessageReceived)
{
    this->onMessageReceived = onMessageReceived;
}

void MosquittoConnection::registerReadyToSendChangedCallback(
        std::function<void(bool)> onReadyToSendChanged)
{
    std::lock_guard<std::mutex> lock(onReadyToSendChangedMutex);
    this->onReadyToSendChanged = std::move(onReadyToSendChanged);
}

bool MosquittoConnection::isSubscribedToChannelTopic() const
{
    return subscribedToChannelTopic;
}

bool MosquittoConnection::isReadyToSend() const
{
    return readyToSend;
}

void MosquittoConnection::on_subscribe(int mid, int qos_count, const int* granted_qos)
{
    JOYNR_LOG_DEBUG(logger(), "Subscribed (mid: {} with granted QOS {}", mid, granted_qos[0]);

    for (int i = 1; i < qos_count; i++) {
        JOYNR_LOG_DEBUG(logger(), "QOS: {} granted {}", i, granted_qos[i]);
    }

    if (mid == subscribeChannelMid) {
        subscribedToChannelTopic = true;
        setReadyToSend(isConnected);
    }
}

void MosquittoConnection::on_message(const mosquitto_message* message)
{
    if (!onMessageReceived) {
        JOYNR_LOG_ERROR(logger(),
                        "Discarding received message, since onMessageReceived callback is empty.");
        return;
    }

    if (!message || message->payloadlen <= 0) {
        JOYNR_LOG_ERROR(
                logger(),
                "Discarding received message: invalid message or non-positive payload's length.");
        return;
    }

    std::uint8_t* data = static_cast<std::uint8_t*>(message->payload);

    // convert address of data into integral type
    std::uintptr_t integralAddress = reinterpret_cast<std::uintptr_t>(data);
    const bool overflow =
            joynr::util::isAdditionOnPointerSafe(integralAddress, message->payloadlen);
    if (overflow) {
        JOYNR_LOG_ERROR(logger(), "Discarding received message, since there is an overflow.");
        return;
    }

    smrf::ByteVector rawMessage(data, data + message->payloadlen);
    onMessageReceived(std::move(rawMessage));
}

void MosquittoConnection::on_publish(int mid)
{
    JOYNR_LOG_TRACE(logger(), "published message with mid {}", std::to_string(mid));
}

void MosquittoConnection::setReadyToSend(bool readyToSend)
{
    if (this->readyToSend != readyToSend) {
        this->readyToSend = readyToSend;

        std::lock_guard<std::mutex> lock(onReadyToSendChangedMutex);
        if (onReadyToSendChanged) {
            onReadyToSendChanged(readyToSend);
        }
    }
}

} // namespace joynr

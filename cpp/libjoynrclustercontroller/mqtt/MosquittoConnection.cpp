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
#include <openssl/opensslv.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

int MosquittoConnection::libUseCount = 0;
std::mutex MosquittoConnection::libUseCountMutex;

MosquittoConnection::MosquittoConnection(const ClusterControllerSettings& ccSettings,
                                         BrokerUrl brokerUrl,
                                         std::chrono::seconds mqttKeepAliveTimeSeconds,
                                         std::chrono::seconds mqttReconnectDelayTimeSeconds,
                                         std::chrono::seconds mqttReconnectMaxDelayTimeSeconds,
                                         bool isMqttExponentialBackoffEnabled,
                                         const std::string& clientId)
        : brokerUrl(brokerUrl),
          mqttKeepAliveTimeSeconds(mqttKeepAliveTimeSeconds),
          mqttReconnectDelayTimeSeconds(mqttReconnectDelayTimeSeconds),
          mqttReconnectMaxDelayTimeSeconds(mqttReconnectMaxDelayTimeSeconds),
          isMqttExponentialBackoffEnabled(isMqttExponentialBackoffEnabled),
          host(brokerUrl.getBrokerChannelsBaseUrl().getHost()),
          port(brokerUrl.getBrokerChannelsBaseUrl().getPort()),
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
          isStopped(true),
          mosq(nullptr)
{
    JOYNR_LOG_INFO(logger(), "Init mosquitto connection using MQTT client ID: {}", clientId);

    {
        std::unique_lock<std::mutex> lock(libUseCountMutex);
        if (libUseCount++ == 0) {
            mosquitto_lib_init();
        }
    }

    mosq = mosquitto_new(clientId.c_str(), false, this);
    if (!mosq) {
        const std::string message = "unable to build mosquitto client";
        JOYNR_LOG_FATAL(logger(), message);
        cleanupLibrary();
        throw joynr::exceptions::JoynrRuntimeException(message);
    }
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_log_callback_set(mosq, on_log);

    if (ccSettings.isMqttUsernameSet()) {
        const std::string mqttUsername = ccSettings.getMqttUsername();
        const char* mqttUsername_cstr = mqttUsername.empty() ? nullptr : mqttUsername.c_str();
        const std::string mqttPassword = ccSettings.getMqttPassword();
        const char* mqttPassword_cstr = mqttPassword.empty() ? nullptr : mqttPassword.c_str();
        int rc = mosquitto_username_pw_set(mosq, mqttUsername_cstr, mqttPassword_cstr);
        if (rc != MOSQ_ERR_SUCCESS) {
            const std::string errorString(getErrorString(rc));
            std::ostringstream messageStringStream;
            messageStringStream << "unable to set username/password for MQTT connection - "
                                << errorString;
            JOYNR_LOG_ERROR(logger(), messageStringStream.str());
            mosquitto_destroy(mosq);
            cleanupLibrary();
            throw joynr::exceptions::JoynrRuntimeException(messageStringStream.str());
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

        int rc = mosquitto_tls_set(mosq,
                                   mqttCertificateAuthorityPemFilename_cstr,
                                   mqttCertificateAuthorityCertificateFolderPath_cstr,
                                   mqttCertificatePemFilename_cstr,
                                   mqttPrivateKeyPemFilename_cstr,
                                   nullptr);

        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(mosq);
            cleanupLibrary();
            const std::string message = "Connection to " + brokerUrl.toString() +
                                        " : Mqtt TLS enabled, but TLS certificates are incorrectly "
                                        "specified or inaccessible: " +
                                        getErrorString(rc);

            JOYNR_LOG_FATAL(logger(), message);

            throw joynr::exceptions::JoynrRuntimeException(message);
        }

        const std::string tlsCiphers = ccSettings.getMqttTlsCiphers();
        rc = mosquitto_tls_opts_set(mosq,
                                    SSL_VERIFY_PEER,
                                    ccSettings.getMqttTlsVersion().c_str(),
                                    tlsCiphers.empty() ? nullptr : tlsCiphers.c_str());

        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(mosq);
            cleanupLibrary();
            const std::string message =
                    "Connection to " + brokerUrl.toString() +
                    " : fatal failure to initialize TLS connection, error settings TLS options:  " +
                    getErrorString(rc);

            JOYNR_LOG_FATAL(logger(), message);
            throw joynr::exceptions::JoynrRuntimeException(message);
        }

#if (defined(MQTT_OCSP_ENABLED) && (LIBMOSQUITTO_VERSION_NUMBER >= 1006000))
        rc = mosquitto_int_option(mosq, MOSQ_OPT_TLS_OCSP_REQUIRED, true);
        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(mosq);
            cleanupLibrary();
            const std::string message =
                    "Connection to " + brokerUrl.toString() +
                    " : fatal failure to require OCSP, error settings TLS options: " +
                    getErrorString(rc);

            JOYNR_LOG_FATAL(logger(), message);
            throw joynr::exceptions::JoynrRuntimeException(message);
        }
        JOYNR_LOG_DEBUG(logger(), "Connection to {} :MQTT OCSP is enabled", brokerUrl.toString());
#else
        JOYNR_LOG_DEBUG(logger(), "Connection to {} :MQTT OCSP is disabled", brokerUrl.toString());
#endif /* MQTT_OCSP_ENABLED */
    } else {
        JOYNR_LOG_DEBUG(
                logger(), "Connection to {}: MQTT connection not encrypted", brokerUrl.toString());
    }
}

// wrappers

void MosquittoConnection::on_connect(struct mosquitto* mosq, void* userdata, int rc)
{
    std::ignore = mosq;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(), "Mosquitto Connection established");
        mosquittoConnection->isConnected = true;

        mosquittoConnection->createSubscriptions();
    } else {
        const std::string errorString(mosquittoConnection->getErrorString(rc));
        JOYNR_LOG_ERROR(
                logger(), "Mosquitto Connection Error: {} ({})", std::to_string(rc), errorString);
        ;
    }
}

void MosquittoConnection::on_disconnect(struct mosquitto* mosq, void* userdata, int rc)
{
    std::ignore = mosq;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    const std::string errorString(getErrorString(rc));
    mosquittoConnection->setReadyToSend(false);
    if (!mosquittoConnection->isConnected) {
        // In case we didn't connect yet
        JOYNR_LOG_ERROR(logger(),
                        "Not yet connected to tcp://{}:{}, error: {}",
                        mosquittoConnection->host,
                        mosquittoConnection->port,
                        errorString);
        return;
    }

    // There was indeed a disconnect...set connect to false
    mosquittoConnection->isConnected = false;

    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(),
                       "Disconnected from tcp://{}:{}",
                       mosquittoConnection->host,
                       mosquittoConnection->port);
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Unexpectedly disconnected from tcp://{}:{}, error: {}",
                        mosquittoConnection->host,
                        mosquittoConnection->port,
                        errorString);
        mosquitto_reconnect(mosq);
    }
}

void MosquittoConnection::on_publish(struct mosquitto* mosq, void* userdata, int mid)
{
    std::ignore = mosq;
    std::ignore = userdata;
    JOYNR_LOG_TRACE(logger(), "published message with mid {}", std::to_string(mid));
}

void MosquittoConnection::on_message(struct mosquitto* mosq,
                                     void* userdata,
                                     const struct mosquitto_message* message)
{
    std::ignore = mosq;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    if (!mosquittoConnection->onMessageReceived) {
        JOYNR_LOG_ERROR(logger(),
                        "Discarding received message, since onMessageReceived callback is empty.");
        return;
    }

    if (!message || message->payloadlen <= 0) {
        JOYNR_LOG_ERROR(logger(),
                        "Discarding received message: invalid message or non-positive "
                        "payload's length.");
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
    mosquittoConnection->onMessageReceived(std::move(rawMessage));
}

void MosquittoConnection::on_subscribe(struct mosquitto* mosq,
                                       void* userdata,
                                       int mid,
                                       int qos_count,
                                       const int* granted_qos)
{
    std::ignore = mosq;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    JOYNR_LOG_DEBUG(logger(), "Subscribed (mid: {} with granted QOS {}", mid, granted_qos[0]);

    for (int i = 1; i < qos_count; i++) {
        JOYNR_LOG_DEBUG(logger(), "QOS: {} granted {}", i, granted_qos[i]);
    }

    if (mid == mosquittoConnection->subscribeChannelMid) {
        mosquittoConnection->subscribedToChannelTopic = true;
        mosquittoConnection->setReadyToSend(mosquittoConnection->isConnected);
    }
}

void MosquittoConnection::on_log(struct mosquitto* mosq, void* userdata, int level, const char* str)
{
    std::ignore = mosq;
    std::ignore = userdata;
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

// end of wrappers

void MosquittoConnection::cleanupLibrary()
{
// Prior to openssl version 1.1.x we must not call mosquitto_lib_cleanup()
// at this time since it might cause memory corruption due to duplicated
// free operations inside older openssl code since openssl is used by
// multiple components.
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    {
        std::unique_lock<std::mutex> lock(libUseCountMutex);
        if (--libUseCount == 0) {
            mosquitto_lib_cleanup();
        }
    }
#endif
}

MosquittoConnection::~MosquittoConnection()
{
    std::lock_guard<std::mutex> stopLocker(stopMutex);
    assert(isStopped);

    cleanupLibrary();
}

std::string MosquittoConnection::getErrorString(int rc)
{
    // Do not use mosquitto_strerror() in case of MOSQ_ERR_ERRNO
    // since it calls the MT-unsafe API strerror()
    if (rc != MOSQ_ERR_ERRNO) {
        return std::string(mosquitto_strerror(rc));
    }

    const int storedErrno = errno;
    return joynr::util::getErrorString(storedErrno);
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
    // do not start/stop in parallel
    std::lock_guard<std::mutex> stopLocker(stopMutex);
    if (!isStopped) {
        JOYNR_LOG_INFO(logger(),
                       "Connection to {}: Mosquitto Connection already started",
                       brokerUrl.toString());
        return;
    }

    JOYNR_LOG_TRACE(logger(),
                    "Connection to {}: Start called with isRunning: {}, isConnected: {}",
                    brokerUrl.toString(),
                    isRunning,
                    isConnected);

    JOYNR_LOG_INFO(logger(),
                   "Connection to {}: Try to connect to tcp://{}:{}",
                   brokerUrl.toString(),
                   host,
                   port);

    mosquitto_connect_async(mosq, host.c_str(), port, mqttKeepAliveTimeSeconds.count());

    mosquitto_reconnect_delay_set(mosq,
                                  mqttReconnectDelayTimeSeconds.count(),
                                  mqttReconnectMaxDelayTimeSeconds.count(),
                                  isMqttExponentialBackoffEnabled);

    startLoop();

    if (isRunning) {
        isStopped = false;
    }
}

void MosquittoConnection::startLoop()
{
    int rc = mosquitto_loop_start(mosq);
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(), "Connection to {}: Mosquitto loop started", brokerUrl.toString());
        isRunning = true;
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "Connection to {}: Mosquitto loop start failed: error: {} ({})",
                        brokerUrl.toString(),
                        std::to_string(rc),
                        errorString);
    }
}

void MosquittoConnection::stop()
{
    // do not start/stop in parallel
    std::lock_guard<std::mutex> stopLocker(stopMutex);
    if (isStopped) {
        JOYNR_LOG_INFO(logger(),
                       "Connection to {}: Mosquitto Connection already stopped",
                       brokerUrl.toString());
        return;
    }

    // disconnect() must be called prior to stopLoop() since
    // otherwise the loop in the mosquitto background thread
    // continues forever and thus the join in stopLoop()
    // blocks indefinitely. This applies even if a connection
    // does not exist yet.

    if (isConnected) {
        int rc = mosquitto_disconnect(mosq);

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
        mosquitto_disconnect(mosq);
        stopLoop();
    }
    setReadyToSend(false);
    isStopped = true;
}

void MosquittoConnection::stopLoop(bool force)
{
    int rc = mosquitto_loop_stop(mosq, force);

    if (rc == MOSQ_ERR_SUCCESS) {
        isRunning = false;
        JOYNR_LOG_INFO(logger(), "Mosquitto loop stopped");
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "Connection to {}: Mosquitto loop stop failed: error: {} ({})",
                        brokerUrl.toString(),
                        std::to_string(rc),
                        errorString);
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
        JOYNR_LOG_ERROR(logger(),
                        "Connection to {}: Error subscribing to Mqtt topic, error: ",
                        brokerUrl.toString(),
                        error.getMessage());
    }
}

void MosquittoConnection::subscribeToTopicInternal(const std::string& topic,
                                                   const bool isChannelTopic)
{
    int* mid = nullptr;
    if (isChannelTopic) {
        mid = &subscribeChannelMid;
    }
    int rc = mosquitto_subscribe(mosq, mid, topic.c_str(), getMqttQos());
    switch (rc) {
    case (MOSQ_ERR_SUCCESS):
        JOYNR_LOG_INFO(logger(), "Connection to {}: Subscribed to {}", brokerUrl.toString(), topic);
        break;
    case (MOSQ_ERR_NO_CONN): {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_DEBUG(logger(),
                        "Connection to {}: Subscription to {} failed: error: {} ({}). "
                        "Subscription will be restored on connect.",
                        brokerUrl.toString(),
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
            int rc = mosquitto_unsubscribe(mosq, nullptr, topic.c_str());
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
    JOYNR_LOG_DEBUG(logger(),
                    "Connection to {}: Publish message of length {} to {}",
                    brokerUrl.toString(),
                    payloadlen,
                    topic);

    int mid;
    int rc = mosquitto_publish(
            mosq, &mid, topic.c_str(), payloadlen, payload, qosLevel, isMqttRetain());
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
    JOYNR_LOG_TRACE(logger(),
                    "Connection to {}: published message with mqtt message id {}",
                    brokerUrl.toString(),
                    std::to_string(mid));
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

BrokerUrl MosquittoConnection::getBrokerUrl() const
{
    return brokerUrl;
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

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

#include "MosquittoConnection.h"

#include <cassert>
#include <cerrno>
#include <sstream>
#include <thread>
#include <tuple>
#include <utility>

#include <openssl/ssl.h>
#include <openssl/opensslv.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Url.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

int MosquittoConnection::_libUseCount = 0;
std::mutex MosquittoConnection::_libUseCountMutex;

MosquittoConnection::MosquittoConnection(const ClusterControllerSettings& ccSettings,
                                         BrokerUrl brokerUrl,
                                         std::chrono::seconds mqttKeepAliveTimeSeconds,
                                         std::chrono::seconds mqttReconnectDelayTimeSeconds,
                                         std::chrono::seconds mqttReconnectMaxDelayTimeSeconds,
                                         bool isMqttExponentialBackoffEnabled,
                                         const std::string& clientId,
                                         const std::string& gbid)
        : _brokerUrl(brokerUrl),
          _mqttKeepAliveTimeSeconds(mqttKeepAliveTimeSeconds),
          _mqttReconnectDelayTimeSeconds(mqttReconnectDelayTimeSeconds),
          _mqttReconnectMaxDelayTimeSeconds(mqttReconnectMaxDelayTimeSeconds),
          _isMqttExponentialBackoffEnabled(isMqttExponentialBackoffEnabled),
          _host(brokerUrl.getBrokerChannelsBaseUrl().getHost()),
          _port(brokerUrl.getBrokerChannelsBaseUrl().getPort()),
          _channelId(),
          _subscribeChannelMid(),
          _topic(),
          _additionalTopics(),
          _additionalTopicsMutex(),
          _isConnected(false),
          _isRunning(false),
          _isChannelIdRegistered(false),
          _subscribedToChannelTopic(false),
          _readyToSend(false),
          _onMessageReceived(),
          _onReadyToSendChangedMutex(),
          _onReadyToSendChanged(),
          _stopMutex(),
          _stopStartMutex(),
          _isStopped(true),
          _isActive(false),
          _restartThreadShutdown(false),
          _restartSemaphore(0),
          _restartThread(),
          _mosq(nullptr),
          _mqttMaximumPacketSize(0),
          _gbid(gbid)
{
    JOYNR_LOG_INFO(
            logger(), "[{}] Init mosquitto connection using MQTT client ID: {}", _gbid, clientId);

    {
        std::unique_lock<std::mutex> lock(_libUseCountMutex);
        if (_libUseCount++ == 0) {
            mosquitto_lib_init();
        }
    }

    _mosq = mosquitto_new(clientId.c_str(), false, this);
    if (!_mosq) {
        const std::string message = fmt::format("[{}] unable to build mosquitto client", _gbid);
        JOYNR_LOG_FATAL(logger(), message);
        cleanupLibrary();
        throw joynr::exceptions::JoynrRuntimeException(message);
    }
    mosquitto_int_option(_mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    mosquitto_connect_v5_callback_set(_mosq, on_connect_v5);
    mosquitto_disconnect_v5_callback_set(_mosq, on_disconnect_v5);
    mosquitto_publish_v5_callback_set(_mosq, on_publish_v5);
    mosquitto_message_v5_callback_set(_mosq, on_message_v5);
    mosquitto_subscribe_v5_callback_set(_mosq, on_subscribe_v5);
    // unsubscribe callback not used
    mosquitto_log_callback_set(_mosq, on_log);

    if (ccSettings.isMqttUsernameSet()) {
        const std::string mqttUsername = ccSettings.getMqttUsername();
        const char* mqttUsername_cstr = mqttUsername.empty() ? nullptr : mqttUsername.c_str();
        const std::string mqttPassword = ccSettings.getMqttPassword();
        const char* mqttPassword_cstr = mqttPassword.empty() ? nullptr : mqttPassword.c_str();
        int rc = mosquitto_username_pw_set(_mosq, mqttUsername_cstr, mqttPassword_cstr);
        if (rc != MOSQ_ERR_SUCCESS) {
            const std::string errorString(getErrorString(rc));
            std::string errorMessage =
                    fmt::format("[{}] unable to set username/password for MQTT connection - {}",
                                _gbid,
                                errorString);
            JOYNR_LOG_ERROR(logger(), errorMessage);
            mosquitto_destroy(_mosq);
            _mosq = nullptr;
            cleanupLibrary();
            throw joynr::exceptions::JoynrRuntimeException(errorMessage);
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

        int rc = mosquitto_tls_set(_mosq,
                                   mqttCertificateAuthorityPemFilename_cstr,
                                   mqttCertificateAuthorityCertificateFolderPath_cstr,
                                   mqttCertificatePemFilename_cstr,
                                   mqttPrivateKeyPemFilename_cstr,
                                   nullptr);

        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(_mosq);
            _mosq = nullptr;
            cleanupLibrary();
            const std::string message =
                    fmt::format("[{}] Connection to {} : Mqtt TLS enabled, but TLS "
                                "certificates are incorrectly specified or inaccessible: {}",
                                _gbid,
                                brokerUrl.toString(),
                                getErrorString(rc));
            JOYNR_LOG_FATAL(logger(), message);

            throw joynr::exceptions::JoynrRuntimeException(message);
        }

        const std::string tlsCiphers = ccSettings.getMqttTlsCiphers();
        rc = mosquitto_tls_opts_set(_mosq,
                                    SSL_VERIFY_PEER,
                                    ccSettings.getMqttTlsVersion().c_str(),
                                    tlsCiphers.empty() ? nullptr : tlsCiphers.c_str());

        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(_mosq);
            _mosq = nullptr;
            cleanupLibrary();
            const std::string message =
                    fmt::format("[{}] Connection to {} : fatal failure to initialize TLS "
                                "connection, error settings TLS options: {}",
                                _gbid,
                                brokerUrl.toString(),
                                getErrorString(rc));
            JOYNR_LOG_FATAL(logger(), message);
            throw joynr::exceptions::JoynrRuntimeException(message);
        }

#if (defined(MQTT_OCSP_ENABLED) && (LIBMOSQUITTO_VERSION_NUMBER >= 1006000))
        rc = mosquitto_int_option(_mosq, MOSQ_OPT_TLS_OCSP_REQUIRED, true);
        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(_mosq);
            _mosq = nullptr;
            cleanupLibrary();
            const std::string message = fmt::format("[{}] Connection to {} : fatal failure to "
                                                    "require OCSP, error settings TLS options: ",
                                                    _gbid,
                                                    brokerUrl.toString(),
                                                    getErrorString(rc));
            JOYNR_LOG_FATAL(logger(), message);
            throw joynr::exceptions::JoynrRuntimeException(message);
        }
        JOYNR_LOG_DEBUG(logger(),
                        "[{}] Connection to {} :MQTT OCSP is enabled",
                        _gbid,
                        brokerUrl.toString());
#else
        JOYNR_LOG_DEBUG(logger(),
                        "[{}] Connection to {} :MQTT OCSP is disabled",
                        _gbid,
                        brokerUrl.toString());
#endif /* MQTT_OCSP_ENABLED */
    } else {
        JOYNR_LOG_DEBUG(logger(),
                        "[{}] Connection to {}: MQTT connection not encrypted",
                        _gbid,
                        brokerUrl.toString());
    }

    _restartThread = std::thread([this]() {
        for (;;) {
            if (_restartThreadShutdown) {
                break;
            }

            JOYNR_LOG_INFO(logger(), "[{}] restartThread: waiting for restartSemaphore", _gbid);
            _restartSemaphore.wait();
            JOYNR_LOG_INFO(logger(), "[{}] restartThread: got notified by restartSemaphore", _gbid);

            if (_restartThreadShutdown) {
                break;
            }

            {
                std::lock_guard<std::mutex> stopStartLocker(_stopStartMutex);
                if (_isActive) {
                    JOYNR_LOG_INFO(logger(), "[{}] restartThread: calling stopInternal()", _gbid);
                    stopInternal();
                } else {
                    JOYNR_LOG_INFO(logger(),
                                   "[{}] restartThread: external comms disabled - skipping stop",
                                   _gbid);
                    continue;
                }
            }

            if (_restartThreadShutdown) {
                break;
            }

            std::this_thread::sleep_for(_mqttReconnectDelayTimeSeconds);

            if (_restartThreadShutdown) {
                break;
            }

            {
                std::lock_guard<std::mutex> stopStartLocker(_stopStartMutex);
                if (_isActive) {
                    JOYNR_LOG_INFO(logger(), "[{}] restartThread: calling startInternal()", _gbid);
                    startInternal();
                } else {
                    JOYNR_LOG_INFO(logger(),
                                   "[{}] restartThread: external comms disabled - skipping restart",
                                   _gbid);
                }
            }
        }
        JOYNR_LOG_INFO(logger(), "[{}] restartThread terminating", _gbid);
    });
}

// wrappers

void MosquittoConnection::on_connect_v5(struct mosquitto* mosq,
                                        void* userdata,
                                        int rc,
                                        int flags,
                                        const mosquitto_property* props)
{
    std::ignore = mosq;
    std::ignore = flags;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;

    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(
                logger(), "[{}] Mosquitto Connection established", mosquittoConnection->_gbid);

        uint32_t tmpMaximumPacketSize;
        if (mosquitto_property_read_int32(
                    props, MQTT_PROP_MAXIMUM_PACKET_SIZE, &tmpMaximumPacketSize, false)) {
            mosquittoConnection->_mqttMaximumPacketSize = tmpMaximumPacketSize;
            JOYNR_LOG_INFO(logger(),
                           "[{}] Maximum packet size = {}",
                           mosquittoConnection->_gbid,
                           tmpMaximumPacketSize);
        } else {
            mosquittoConnection->_mqttMaximumPacketSize = 0;
            JOYNR_LOG_INFO(
                    logger(), "[{}] Maximum packet size = unlimited", mosquittoConnection->_gbid);
        }

        mosquittoConnection->_isConnected = true;

        mosquittoConnection->createSubscriptions();
    } else {
        const std::string errorString(mosquittoConnection->getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Mosquitto Connection Error: {} ({})",
                        mosquittoConnection->_gbid,
                        std::to_string(rc),
                        errorString);
    }
}

void MosquittoConnection::on_disconnect_v5(struct mosquitto* mosq,
                                           void* userdata,
                                           int rc,
                                           const mosquitto_property* props)
{
    std::ignore = mosq;
    std::ignore = props;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    const int storedErrno = errno;
    const std::string errorString(getErrorString(rc));
    mosquittoConnection->setReadyToSend(false);
    if (!mosquittoConnection->_isConnected) {
        // In case we didn't connect yet
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Not yet connected to tcp://{}:{}, rc: {}, errno: {}, error: {}",
                        mosquittoConnection->_gbid,
                        mosquittoConnection->_host,
                        mosquittoConnection->_port,
                        rc,
                        errno,
                        errorString);
        return;
    }

    // There was indeed a disconnect...set connect to false
    mosquittoConnection->_isConnected = false;

    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(),
                       "[{}] Disconnected from tcp://{}:{}",
                       mosquittoConnection->_gbid,
                       mosquittoConnection->_host,
                       mosquittoConnection->_port);
        JOYNR_LOG_INFO(logger(), "[{}] Triggering loop restart", mosquittoConnection->_gbid);
        mosquittoConnection->_restartSemaphore.notify();
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Unexpectedly disconnected from tcp://{}:{}, rc: {}, errno: {}, "
                        "error: {}",
                        mosquittoConnection->_gbid,
                        mosquittoConnection->_host,
                        mosquittoConnection->_port,
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
            JOYNR_LOG_INFO(logger(), "[{}] Triggering loop restart", mosquittoConnection->_gbid);
            mosquittoConnection->_restartSemaphore.notify();
            return;
        case MOSQ_ERR_ERRNO:
            if (storedErrno == EPROTO) {
                JOYNR_LOG_INFO(
                        logger(), "[{}] Triggering loop restart", mosquittoConnection->_gbid);
                mosquittoConnection->_restartSemaphore.notify();
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
        mosquitto_reconnect(mosq);
    }
}

void MosquittoConnection::on_publish_v5(struct mosquitto* mosq,
                                        void* userdata,
                                        int mid,
                                        int reason_code,
                                        const mosquitto_property* props)
{
    std::ignore = mosq;
    std::ignore = reason_code;
    std::ignore = props;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    JOYNR_LOG_TRACE(logger(),
                    "[{}] published message with mid {}",
                    mosquittoConnection->_gbid,
                    std::to_string(mid));
}

void MosquittoConnection::on_message_v5(struct mosquitto* mosq,
                                        void* userdata,
                                        const struct mosquitto_message* message,
                                        const mosquitto_property* props)
{
    std::ignore = mosq;
    std::ignore = props;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    if (!mosquittoConnection->_onMessageReceived) {
        JOYNR_LOG_ERROR(
                logger(),
                "[{}] Discarding received message, since onMessageReceived callback is empty.",
                mosquittoConnection->_gbid);
        return;
    }

    if (!message || message->payloadlen <= 0) {
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Discarding received message: invalid message or non-positive "
                        "payload's length.",
                        mosquittoConnection->_gbid);
        return;
    }

    std::uint8_t* data = static_cast<std::uint8_t*>(message->payload);

    // convert address of data into integral type
    std::uintptr_t integralAddress = reinterpret_cast<std::uintptr_t>(data);
    const bool overflow =
            joynr::util::isAdditionOnPointerCausesOverflow(integralAddress, message->payloadlen);
    if (overflow) {
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Discarding received message, since there is an overflow.",
                        mosquittoConnection->_gbid);
        return;
    }

    smrf::ByteVector rawMessage(data, data + message->payloadlen);
    mosquittoConnection->_onMessageReceived(std::move(rawMessage));
}

void MosquittoConnection::on_subscribe_v5(struct mosquitto* mosq,
                                          void* userdata,
                                          int mid,
                                          int qos_count,
                                          const int* granted_qos,
                                          const mosquitto_property* props)
{
    std::ignore = mosq;
    std::ignore = props;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    JOYNR_LOG_DEBUG(logger(),
                    "[{}] Subscribed (mid: {} with granted QOS {}",
                    mosquittoConnection->_gbid,
                    mid,
                    granted_qos[0]);

    for (int i = 1; i < qos_count; i++) {
        JOYNR_LOG_DEBUG(
                logger(), "[{}] QOS: {} granted {}", i, mosquittoConnection->_gbid, granted_qos[i]);
    }

    if (mid == mosquittoConnection->_subscribeChannelMid) {
        mosquittoConnection->_subscribedToChannelTopic = true;
        mosquittoConnection->setReadyToSend(mosquittoConnection->_isConnected);
    }
}

void MosquittoConnection::on_log(struct mosquitto* mosq, void* userdata, int level, const char* str)
{
    std::ignore = mosq;
    class MosquittoConnection* mosquittoConnection = (class MosquittoConnection*)userdata;
    if (level == MOSQ_LOG_ERR) {
        JOYNR_LOG_ERROR(logger(), "[{}] Mosquitto Log: {}", mosquittoConnection->_gbid, str);
    } else if (level == MOSQ_LOG_WARNING) {
        JOYNR_LOG_WARN(logger(), "[{}] Mosquitto Log: {}", mosquittoConnection->_gbid, str);
    } else if (level == MOSQ_LOG_INFO) {
        JOYNR_LOG_INFO(logger(), "[{}] Mosquitto Log: {}", mosquittoConnection->_gbid, str);
    } else {
        // MOSQ_LOG_NOTICE || MOSQ_LOG_DEBUG || any other log level
        JOYNR_LOG_DEBUG(logger(), "[{}] Mosquitto Log: {}", mosquittoConnection->_gbid, str);
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
        std::unique_lock<std::mutex> lock(_libUseCountMutex);
        if (_libUseCount && --_libUseCount == 0) {
            mosquitto_lib_cleanup();
        }
    }
#endif
}

MosquittoConnection::~MosquittoConnection()
{
    std::lock_guard<std::mutex> stopLocker(_stopMutex);
    assert(_isStopped);
    assert(!_isActive);

    // signal restartThread, we are about to shutdown
    _restartThreadShutdown = true;
    _restartSemaphore.notify();
    if (_restartThread.joinable()) {
        JOYNR_LOG_INFO(logger(), "[{}] restartThread joinable", _gbid);
        _restartThread.join();
        JOYNR_LOG_INFO(logger(), "[{}] restartThread joined", _gbid);
    } else {
        JOYNR_LOG_ERROR(logger(), "[{}] restartThread not joinable", _gbid);
    }

    if (_mosq) {
        mosquitto_destroy(_mosq);
        _mosq = nullptr;
    }
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
    return _mqttQos;
}

std::string MosquittoConnection::getMqttPrio() const
{
    static const std::string value("low");
    return value;
}

bool MosquittoConnection::isMqttRetain() const
{
    return _mqttRetain;
}

void MosquittoConnection::start()
{
    JOYNR_LOG_INFO(logger(), "[{}] MosquittoConnection external start() called", _gbid);
    std::lock_guard<std::mutex> stopStartLocker(_stopStartMutex);
    startInternal();
    _isActive = true;
}

void MosquittoConnection::startInternal()
{
    // do not start/stop in parallel
    std::lock_guard<std::mutex> stopLocker(_stopMutex);
    if (!_isStopped) {
        JOYNR_LOG_INFO(logger(),
                       "[{}] Connection to {}: Mosquitto Connection already started",
                       _gbid,
                       _brokerUrl.toString());
        return;
    }

    JOYNR_LOG_TRACE(logger(),
                    "[{}] Connection to {}: Start called with isRunning: {}, isConnected: {}",
                    _gbid,
                    _brokerUrl.toString(),
                    _isRunning,
                    _isConnected);

    JOYNR_LOG_INFO(logger(),
                   "[{}] Connection to {}: Try to connect to tcp://{}:{}",
                   _gbid,
                   _brokerUrl.toString(),
                   _host,
                   _port);

    mosquitto_property* props = nullptr;
    mosquitto_property_add_int32(
            &props, MQTT_PROP_SESSION_EXPIRY_INTERVAL, std::numeric_limits<std::int32_t>::max());
    mosquitto_connect_bind_async_v5(
            _mosq, _host.c_str(), _port, _mqttKeepAliveTimeSeconds.count(), nullptr, props);
    mosquitto_property_free_all(&props);

    mosquitto_reconnect_delay_set(_mosq,
                                  _mqttReconnectDelayTimeSeconds.count(),
                                  _mqttReconnectMaxDelayTimeSeconds.count(),
                                  _isMqttExponentialBackoffEnabled);

    startLoop();

    if (_isRunning) {
        _isStopped = false;
    }
}

void MosquittoConnection::startLoop()
{
    int rc = mosquitto_loop_start(_mosq);
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_INFO(logger(),
                       "[{}] Connection to {}: Mosquitto loop started",
                       _gbid,
                       _brokerUrl.toString());
        _isRunning = true;
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Connection to {}: Mosquitto loop start failed: error: {} ({})",
                        _gbid,
                        _brokerUrl.toString(),
                        std::to_string(rc),
                        errorString);
    }
}

void MosquittoConnection::stop()
{
    JOYNR_LOG_INFO(logger(), "[{}] MosquittoConnection external stop() called", _gbid);
    std::lock_guard<std::mutex> stopStartLocker(_stopStartMutex);
    stopInternal();
    _isActive = false;
}

void MosquittoConnection::stopInternal()
{
    // do not start/stop in parallel
    std::lock_guard<std::mutex> stopLocker(_stopMutex);
    if (_isStopped) {
        JOYNR_LOG_INFO(logger(),
                       "[{}] Connection to {}: Mosquitto Connection already stopped",
                       _gbid,
                       _brokerUrl.toString());
        return;
    }

    // disconnect() must be called prior to stopLoop() since
    // otherwise the loop in the mosquitto background thread
    // continues forever and thus the join in stopLoop()
    // blocks indefinitely. This applies even if a connection
    // does not exist yet.
    const mosquitto_property* props = nullptr;
    const int reason_code = 0;

    if (_isConnected) {
        int rc = mosquitto_disconnect_v5(_mosq, reason_code, props);

        if (rc == MOSQ_ERR_SUCCESS) {
            JOYNR_LOG_INFO(logger(), "[{}] Mosquitto Connection disconnected", _gbid);
        } else {
            const std::string errorString(getErrorString(rc));
            JOYNR_LOG_ERROR(logger(),
                            "[{}] Mosquitto disconnect failed: error: {} ({})",
                            _gbid,
                            std::to_string(rc),
                            errorString);
        }
        stopLoop();
    } else if (_isRunning) {
        mosquitto_disconnect_v5(_mosq, reason_code, props);
        stopLoop();
    }
    setReadyToSend(false);
    _isStopped = true;
}

void MosquittoConnection::stopLoop(bool force)
{
    int rc = mosquitto_loop_stop(_mosq, force);

    if (rc == MOSQ_ERR_SUCCESS) {
        _isRunning = false;
        JOYNR_LOG_INFO(logger(), "[{}] Mosquitto loop stopped", _gbid);
    } else {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Connection to {}: Mosquitto loop stop failed: error: {} ({})",
                        _gbid,
                        _brokerUrl.toString(),
                        std::to_string(rc),
                        errorString);
    }
}

void MosquittoConnection::createSubscriptions()
{
    while (!_isChannelIdRegistered && _isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    try {
        subscribeToTopicInternal(_topic, true);
        std::lock_guard<std::recursive_mutex> lock(_additionalTopicsMutex);
        for (const std::string& additionalTopic : _additionalTopics) {
            subscribeToTopicInternal(additionalTopic);
        }
    } catch (const exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "[{}] Connection to {}: Error subscribing to Mqtt topic, error: ",
                        _gbid,
                        _brokerUrl.toString(),
                        error.getMessage());
    }
}

void MosquittoConnection::subscribeToTopicInternal(const std::string& topic,
                                                   const bool isChannelTopic)
{
    int* mid = nullptr;
    if (isChannelTopic) {
        mid = &_subscribeChannelMid;
    }
    const int options = 0;
    const mosquitto_property* props = nullptr;
    int rc = mosquitto_subscribe_v5(_mosq, mid, topic.c_str(), getMqttQos(), options, props);
    switch (rc) {
    case (MOSQ_ERR_SUCCESS):
        JOYNR_LOG_INFO(logger(),
                       "[{}] Connection to {}: Subscribed to {}",
                       _gbid,
                       _brokerUrl.toString(),
                       topic);
        break;
    case (MOSQ_ERR_NO_CONN): {
        const std::string errorString(getErrorString(rc));
        JOYNR_LOG_DEBUG(logger(),
                        "[{}] Connection to {}: Subscription to {} failed: error: {} ({}). "
                        "Subscription will be restored on connect.",
                        _gbid,
                        _brokerUrl.toString(),
                        topic,
                        std::to_string(rc),
                        errorString);
        break;
    }
    default: {
        // MOSQ_ERR_INVAL, MOSQ_ERR_NOMEM
        const std::string errorString(getErrorString(rc));
        std::string errorMsg = fmt::format("[{}] Subscription to {} failed: error: {} ({})",
                                           _gbid,
                                           topic,
                                           std::to_string(rc),
                                           errorString);
        throw exceptions::JoynrRuntimeException(errorMsg);
    }
    }
}

void MosquittoConnection::subscribeToTopic(const std::string& topic)
{
    if (!_isChannelIdRegistered) {
        std::string errorMsg = fmt::format(
                "[{}] No channelId registered, cannot subscribe to topic {}", _gbid, topic);
        throw exceptions::JoynrRuntimeException(errorMsg);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(_additionalTopicsMutex);
        if (_additionalTopics.find(topic) != _additionalTopics.end()) {
            JOYNR_LOG_DEBUG(logger(), "[{}] Already subscribed to topic {}", _gbid, topic);
            return;
        }

        subscribeToTopicInternal(topic);

        _additionalTopics.insert(topic);
    }
}

void MosquittoConnection::unsubscribeFromTopic(const std::string& topic)
{
    if (_isChannelIdRegistered) {
        std::lock_guard<std::recursive_mutex> lock(_additionalTopicsMutex);
        if (_additionalTopics.find(topic) == _additionalTopics.end()) {
            JOYNR_LOG_DEBUG(
                    logger(), "[{}] Unsubscribe called for non existing topic {}", _gbid, topic);
            return;
        }
        _additionalTopics.erase(topic);
        if (_isConnected && _isRunning) {
            const mosquitto_property* props = nullptr;
            int rc = mosquitto_unsubscribe_v5(_mosq, nullptr, topic.c_str(), props);
            if (rc == MOSQ_ERR_SUCCESS) {
                JOYNR_LOG_INFO(logger(), "[{}] Unsubscribed from {}", _gbid, topic);
            } else {
                // MOSQ_ERR_INVAL || MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN
                const std::string errorString(getErrorString(rc));
                JOYNR_LOG_ERROR(logger(),
                                "[{}] Unsubscribe from {} failed: error: {} ({})",
                                _gbid,
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
        const std::uint32_t msgTtlSec,
        const uint32_t payloadlen = 0,
        const void* payload = nullptr)
{
    JOYNR_LOG_DEBUG(logger(),
                    "[{}] Connection to {}: Publish message of length {} to {}",
                    _gbid,
                    _brokerUrl.toString(),
                    payloadlen,
                    topic);

    int mid;
    mosquitto_property* props = nullptr;
    int ret = mosquitto_property_add_int32(&props, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, msgTtlSec);
    switch (ret) {
    case MOSQ_ERR_SUCCESS:
        JOYNR_LOG_TRACE(
                logger(), "[{}] Added MQTT message expiry date in Sec {}", _gbid, msgTtlSec);
        break;
    default:
        // MOSQ_ERR_INVAL, MOSQ_ERR_NOMEM
        const std::string errorString(getErrorString(ret));
        std::string errorMsg = fmt::format(
                "[{}] Adding MQTT message expiry interval property failed: error: {} ({})",
                _gbid,
                std::to_string(ret),
                errorString);
        mosquitto_property_free_all(&props);
        throw exceptions::JoynrRuntimeException(errorMsg);
    }
    int rc = mosquitto_publish_v5(_mosq,
                                  &mid,
                                  topic.c_str(),
                                  static_cast<std::int32_t>(payloadlen),
                                  payload,
                                  qosLevel,
                                  isMqttRetain(),
                                  props);
    mosquitto_property_free_all(&props);
    if (!(rc == MOSQ_ERR_SUCCESS)) {
        const std::string errorString(getErrorString(rc));
        if (rc == MOSQ_ERR_INVAL || rc == MOSQ_ERR_PAYLOAD_SIZE) {
            std::string errorMessage = fmt::format("[{}] message could not be sent: mid (mqtt "
                                                   "message id): {}, error: {} ({})",
                                                   _gbid,
                                                   std::to_string(mid),
                                                   std::to_string(rc),
                                                   errorString);
            onFailure(exceptions::JoynrMessageNotSentException(errorMessage));
            return;
        }
        // MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN || MOSQ_ERR_PROTOCOL ||| unexpected errors
        std::string errorMessage =
                fmt::format("[{}] error sending message: mid (mqtt message id): {}, error: {} ({})",
                            _gbid,
                            std::to_string(mid),
                            std::to_string(rc),
                            errorString);
        onFailure(exceptions::JoynrDelayMessageException(errorMessage));
        return;
    }
    JOYNR_LOG_TRACE(logger(),
                    "[{}] Connection to {}: published message with mqtt message id {}",
                    _gbid,
                    _brokerUrl.toString(),
                    std::to_string(mid));
}

void MosquittoConnection::registerChannelId(const std::string& channelId)
{
    this->_channelId = channelId;
    _topic = channelId + "/" + getMqttPrio() + "/" + "#";
    _isChannelIdRegistered = true;
}

void MosquittoConnection::registerReceiveCallback(
        std::function<void(smrf::ByteVector&&)> onMessageReceived)
{
    this->_onMessageReceived = onMessageReceived;
}

void MosquittoConnection::registerReadyToSendChangedCallback(
        std::function<void(bool)> onReadyToSendChanged)
{
    std::lock_guard<std::mutex> lock(_onReadyToSendChangedMutex);
    this->_onReadyToSendChanged = std::move(onReadyToSendChanged);
}

bool MosquittoConnection::isSubscribedToChannelTopic() const
{
    return _subscribedToChannelTopic;
}

bool MosquittoConnection::isReadyToSend() const
{
    return _readyToSend;
}

void MosquittoConnection::setReadyToSend(bool readyToSend)
{
    if (this->_readyToSend != readyToSend) {
        this->_readyToSend = readyToSend;

        std::lock_guard<std::mutex> lock(_onReadyToSendChangedMutex);
        if (_onReadyToSendChanged) {
            _onReadyToSendChanged(readyToSend);
        }
    }
}

std::uint32_t MosquittoConnection::getMqttMaximumPacketSize() const
{
    return _mqttMaximumPacketSize;
}

} // namespace joynr

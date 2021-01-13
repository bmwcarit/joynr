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
#include "joynr/MessagingSettings.h"

#include <cassert>

#include "joynr/BrokerUrl.h"
#include "joynr/Settings.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

MessagingSettings::MessagingSettings(Settings& settings) : _settings(settings)
{
    settings.fillEmptySettingsWithDefaults(DEFAULT_MESSAGING_SETTINGS_FILENAME());
    checkSettings();
}

MessagingSettings::MessagingSettings(const MessagingSettings& other) : _settings(other._settings)
{
    checkSettings();
}

const std::string& MessagingSettings::SETTING_BROKER_URL()
{
    static const std::string value("messaging/broker-url");
    return value;
}

const std::string& MessagingSettings::SETTING_GBID()
{
    static const std::string value("messaging/gbid");
    return value;
}

std::string MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(std::uint8_t index)
{
    static const std::string valueSection("messaging/additional-backend-");
    static const std::string valueName("-broker-url");
    const std::string concatenatedValue = valueSection + std::to_string(index) + valueName;
    return concatenatedValue;
}

std::string MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(std::uint8_t index)
{
    static const std::string valueSection("messaging/additional-backend-");
    static const std::string valueName("-gbid");
    const std::string concatenatedValue = valueSection + std::to_string(index) + valueName;
    return concatenatedValue;
}

std::string MessagingSettings::SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(
        std::uint8_t index)
{
    static const std::string valueSection("messaging/additional-backend-");
    static const std::string valueName("-mqtt-keep-alive-time-seconds");
    const std::string concatenatedValue = valueSection + std::to_string(index) + valueName;
    return concatenatedValue;
}

std::string MessagingSettings::SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(
        std::uint8_t index)
{
    static const std::string valueSection("messaging/additional-backend-");
    static const std::string valueName("-mqtt-connection-timeout-ms");
    const std::string concatenatedValue = valueSection + std::to_string(index) + valueName;
    return concatenatedValue;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()
{
    static const std::string value("messaging/discovery-directories-domain");
    return value;
}

const std::string& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()
{
    static const std::string value("messaging/capabilities-directory-channelid");
    return value;
}

const std::string& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()
{
    static const std::string value("messaging/capabilities-directory-participantid");
    return value;
}

const std::string& MessagingSettings::SETTING_CERTIFICATE_AUTHORITY()
{
    static const std::string value("messaging/certificate-authority");
    return value;
}

const std::string& MessagingSettings::SETTING_CLIENT_CERTIFICATE()
{
    static const std::string value("messaging/client-certificate");
    return value;
}

const std::string& MessagingSettings::SETTING_CLIENT_CERTIFICATE_PASSWORD()
{
    static const std::string value("messaging/client-certificate-password");
    return value;
}

const std::string& MessagingSettings::SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()
{
    static const std::string value("messaging/mqtt-keep-alive-time-seconds");
    return value;
}

std::chrono::seconds MessagingSettings::DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS()
{
    static const std::chrono::seconds value(60);
    return value;
}

const std::string& MessagingSettings::SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()
{
    static const std::string value("messaging/mqtt-reconnect-delay-time-seconds");
    return value;
}

const std::string& MessagingSettings::SETTING_MQTT_RECONNECT_MAX_DELAY()
{
    static const std::string value("messaging/mqtt-reconnect-max-delay");
    return value;
}

const std::string& MessagingSettings::SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED()
{
    static const std::string value("messaging/mqtt-exponential-backoff-enabled");
    return value;
}

const std::string& MessagingSettings::SETTING_MQTT_CONNECTION_TIMEOUT_MS()
{
    static const std::string value("messaging/mqtt-connection-timeout-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS()
{
    static const std::string value("messaging/discard-unroutable-replies-and-publications");
    return value;
}

std::chrono::milliseconds MessagingSettings::DEFAULT_MQTT_CONNECTION_TIMEOUT_MS()
{
    static const std::chrono::milliseconds value(1000);
    return value;
}

std::chrono::seconds MessagingSettings::DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS()
{
    static const std::chrono::seconds value(1);
    return value;
}

bool MessagingSettings::DEFAULT_MQTT_ENABLED()
{
    static const bool value = false;
    return value;
}

const std::string& MessagingSettings::SETTING_INDEX()
{
    static const std::string value("messaging/index");
    return value;
}

const std::string& MessagingSettings::SETTING_SEND_MSG_RETRY_INTERVAL()
{
    static const std::string value("messaging/send-msg-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS()
{
    static const std::string value("messaging/discovery-entry-expiry-interval-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_PERSISTENCE_FILENAME()
{
    static const std::string value("messaging/persistence-file");
    return value;
}

std::string MessagingSettings::getCertificateAuthority() const
{
    return _settings.get<std::string>(SETTING_CERTIFICATE_AUTHORITY());
}

void MessagingSettings::setCertificateAuthority(const std::string& certificateAuthority)
{
    _settings.set(SETTING_CERTIFICATE_AUTHORITY(), certificateAuthority);
}

std::string MessagingSettings::getClientCertificate() const
{
    return _settings.get<std::string>(SETTING_CLIENT_CERTIFICATE());
}

void MessagingSettings::setClientCertificate(const std::string& clientCertificate)
{
    _settings.set(SETTING_CLIENT_CERTIFICATE(), clientCertificate);
}

std::string MessagingSettings::getClientCertificatePassword() const
{
    return _settings.get<std::string>(SETTING_CLIENT_CERTIFICATE_PASSWORD());
}

void MessagingSettings::setClientCertificatePassword(const std::string& clientCertificatePassword)
{
    _settings.set(SETTING_CLIENT_CERTIFICATE_PASSWORD(), clientCertificatePassword);
}

const std::string& MessagingSettings::DEFAULT_GBID()
{
    static const std::string value("joynrdefaultgbid");
    return value;
}

const std::string& MessagingSettings::DEFAULT_MESSAGING_SETTINGS_FILENAME()
{
    static const std::string value("default-messaging.settings");
    return value;
}

const std::string& MessagingSettings::DEFAULT_PERSISTENCE_FILENAME()
{
    static const std::string value("joynr.settings");
    return value;
}

const std::string& MessagingSettings::SETTING_BROKER_TIMEOUT_MS()
{
    static const std::string value("messaging/broker-timeout");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS()
{
    static const std::string value("messaging/discovery-default-timeout-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS()
{
    static const std::string value("messaging/discovery-default-retry-interval-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_ROUTING_TABLE_GRACE_PERIOD_MS()
{
    static const std::string value("messaging/routing-table-grace-period-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS()
{
    static const std::string value("messaging/routing-table-cleanup-interval-ms");
    return value;
}

std::int64_t MessagingSettings::DEFAULT_BROKER_TIMEOUT_MS()
{
    // 20 seconds
    return (20 * 1000);
}

std::int64_t MessagingSettings::DEFAULT_DISCOVERY_DEFAULT_TIMEOUT_MS()
{
    // 10 minutes
    return 10 * 60 * 1000;
}

std::int64_t MessagingSettings::DEFAULT_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS()
{
    // 10 seconds
    return 10 * 1000;
}

std::int64_t MessagingSettings::DEFAULT_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS()
{
    // 6 weeks
    return 6 * 7 * 24 * 60 * 60 * 1000L;
}

const std::string& MessagingSettings::SETTING_MAXIMUM_TTL_MS()
{
    static const std::string value("messaging/max-ttl-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_MESSAGES_TTL_MS()
{
    static const std::string value("messaging/discovery-messages-ttl");
    return value;
}

std::int64_t MessagingSettings::DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS()
{
    // 40 seconds
    return (40 * 1000);
}

std::int64_t MessagingSettings::DEFAULT_ROUTING_TABLE_GRACE_PERIOD_MS()
{
    // 60 seconds
    return (60 * 1000);
}

std::int64_t MessagingSettings::DEFAULT_ROUTING_TABLE_CLEANUP_INTERVAL_MS()
{
    // 60 seconds
    return (60 * 1000);
}

const std::string& MessagingSettings::SETTING_SEND_MESSAGE_MAX_TTL()
{
    static const std::string value("messaging/max-send-ttl");
    return value;
}

std::int64_t MessagingSettings::DEFAULT_SEND_MESSAGE_MAX_TTL()
{
    // 10 minutes
    return (10 * 60 * 1000);
}

std::uint64_t MessagingSettings::DEFAULT_TTL_UPLIFT_MS()
{
    return 0;
}

std::uint64_t MessagingSettings::DEFAULT_MAXIMUM_TTL_MS()
{
    // 30 days
    return (30UL * 24UL * 60UL * 60UL * 1000UL);
}

bool MessagingSettings::DEFAULT_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS()
{
    static const bool value = false;
    return value;
}

const std::string& MessagingSettings::SETTING_TTL_UPLIFT_MS()
{
    static const std::string value("messaging/ttl-uplift-ms");
    return value;
}

BrokerUrl MessagingSettings::getBrokerUrl() const
{
    return BrokerUrl(_settings.get<std::string>(SETTING_BROKER_URL()));
}

std::string MessagingSettings::getBrokerUrlString() const
{
    return _settings.get<std::string>(SETTING_BROKER_URL());
}

void MessagingSettings::setBrokerUrl(const BrokerUrl& brokerUrl)
{
    const std::string url = brokerUrl.getBrokerChannelsBaseUrl().toString();
    _settings.set(SETTING_BROKER_URL(), url);
}

std::string MessagingSettings::getGbid() const
{
    return _settings.get<std::string>(SETTING_GBID());
}

void MessagingSettings::setGbid(const std::string& gbid)
{
    _settings.set(SETTING_GBID(), gbid);
}

BrokerUrl MessagingSettings::getAdditionalBackendBrokerUrl(std::uint8_t index) const
{
    return BrokerUrl(_settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)));
}

std::string MessagingSettings::getAdditionalBackendBrokerUrlString(std::uint8_t index) const
{
    return _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index));
}

void MessagingSettings::setAdditionalBackendBrokerUrl(const BrokerUrl& brokerUrl,
                                                      std::uint8_t index)
{
    const std::string url = brokerUrl.getBrokerChannelsBaseUrl().toString();
    _settings.set(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index), url);
}

std::string MessagingSettings::getAdditionalBackendGbid(std::uint8_t index) const
{
    return _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_GBID(index));
}

void MessagingSettings::setAdditionalBackendGbid(const std::string& gbid, std::uint8_t index)
{
    _settings.set(SETTING_ADDITIONAL_BACKEND_GBID(index), gbid);
}

std::chrono::seconds MessagingSettings::getAdditionalBackendMqttKeepAliveTimeSeconds(
        std::uint8_t index) const
{
    return std::chrono::seconds(_settings.get<std::int64_t>(
            SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index)));
}

void MessagingSettings::setAdditionalBackendMqttKeepAliveTimeSeconds(
        std::chrono::seconds mqttKeepAliveTimeSeconds,
        std::uint8_t index)
{
    _settings.set(SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index),
                  mqttKeepAliveTimeSeconds.count());
}

std::chrono::milliseconds MessagingSettings::getAdditionalBackendMqttConnectionTimeoutMs(
        std::uint8_t index) const
{
    return std::chrono::milliseconds(_settings.get<std::int64_t>(
            SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index)));
}

std::string MessagingSettings::getDiscoveryDirectoriesDomain() const
{
    return _settings.get<std::string>(SETTING_DISCOVERY_DIRECTORIES_DOMAIN());
}

std::string MessagingSettings::getCapabilitiesDirectoryChannelId() const
{
    return _settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_CHANNELID());
}

std::string MessagingSettings::getCapabilitiesDirectoryParticipantId() const
{
    return _settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID());
}

std::chrono::seconds MessagingSettings::getMqttKeepAliveTimeSeconds() const
{
    return std::chrono::seconds(
            _settings.get<std::int64_t>(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()));
}

void MessagingSettings::setMqttKeepAliveTimeSeconds(std::chrono::seconds mqttKeepAliveTimeSeconds)
{
    _settings.set(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS(), mqttKeepAliveTimeSeconds.count());
}

std::chrono::seconds MessagingSettings::getMqttReconnectDelayTimeSeconds() const
{
    return std::chrono::seconds(
            _settings.get<std::int64_t>(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()));
}

void MessagingSettings::setMqttReconnectDelayTimeSeconds(
        std::chrono::seconds mqttReconnectDelayTimeSeconds)
{
    _settings.set(
            SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS(), mqttReconnectDelayTimeSeconds.count());
}

std::chrono::seconds MessagingSettings::getMqttReconnectMaxDelayTimeSeconds() const
{
    return std::chrono::seconds(_settings.get<std::int64_t>(SETTING_MQTT_RECONNECT_MAX_DELAY()));
}

void MessagingSettings::setMqttReconnectMaxDelayTimeSeconds(
        std::chrono::seconds mqttReconnectMaxDelayTimeSeconds)
{
    _settings.set(SETTING_MQTT_RECONNECT_MAX_DELAY(), mqttReconnectMaxDelayTimeSeconds.count());
}

std::chrono::milliseconds MessagingSettings::getMqttConnectionTimeoutMs() const
{
    return std::chrono::milliseconds(
            _settings.get<std::int64_t>(SETTING_MQTT_CONNECTION_TIMEOUT_MS()));
}

std::int64_t MessagingSettings::getIndex() const
{
    return _settings.get<std::int64_t>(SETTING_INDEX());
}

void MessagingSettings::setIndex(std::int64_t index)
{
    _settings.set(SETTING_INDEX(), index);
}

std::int64_t MessagingSettings::getDiscoveryEntryExpiryIntervalMs() const
{
    return _settings.get<std::int64_t>(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS());
}

void MessagingSettings::setDiscoveryEntryExpiryIntervalMs(std::int64_t expiryIntervalMs)
{
    _settings.set(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS(), expiryIntervalMs);
}

std::uint32_t MessagingSettings::getSendMsgRetryInterval() const
{
    return _settings.get<std::uint32_t>(SETTING_SEND_MSG_RETRY_INTERVAL());
}

void MessagingSettings::setSendMsgRetryInterval(const std::uint32_t& retryInterval)
{
    _settings.set(SETTING_SEND_MSG_RETRY_INTERVAL(), retryInterval);
}

bool MessagingSettings::getMqttExponentialBackoffEnabled() const
{
    return _settings.get<bool>(SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED());
}

void MessagingSettings::setMqttExponentialBackoffEnabled(const bool& enable)
{
    _settings.set(SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED(), enable);
}

std::string MessagingSettings::getMessagingPropertiesPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_PERSISTENCE_FILENAME());
}

void MessagingSettings::setMessagingPropertiesPersistenceFilename(const std::string& filename)
{
    _settings.set(SETTING_PERSISTENCE_FILENAME(), filename);
}

std::int64_t MessagingSettings::getBrokerTimeoutMs() const
{
    return _settings.get<std::int64_t>(SETTING_BROKER_TIMEOUT_MS());
}

void MessagingSettings::setBrokerTimeoutMs(std::int64_t timeout_ms)
{
    _settings.set(SETTING_BROKER_TIMEOUT_MS(), timeout_ms);
}

std::uint64_t MessagingSettings::getMaximumTtlMs() const
{
    return _settings.get<std::uint64_t>(SETTING_MAXIMUM_TTL_MS());
}

void MessagingSettings::setMaximumTtlMs(std::uint64_t maximumTtlMs)
{
    _settings.set(SETTING_MAXIMUM_TTL_MS(), maximumTtlMs);
}

std::int64_t MessagingSettings::getDiscoveryMessagesTtl() const
{
    return _settings.get<std::int64_t>(SETTING_DISCOVERY_MESSAGES_TTL_MS());
}

void MessagingSettings::setDiscoveryMessagesTtl(std::int64_t ttl_ms)
{
    _settings.set(SETTING_DISCOVERY_MESSAGES_TTL_MS(), ttl_ms);
}

std::int64_t MessagingSettings::getSendMsgMaxTtl() const
{
    return _settings.get<std::int64_t>(SETTING_SEND_MESSAGE_MAX_TTL());
}

void MessagingSettings::setSendMsgMaxTtl(std::int64_t ttl_ms)
{
    _settings.set(SETTING_SEND_MESSAGE_MAX_TTL(), ttl_ms);
}

void MessagingSettings::setTtlUpliftMs(std::uint64_t ttlUpliftMs)
{
    _settings.set(SETTING_TTL_UPLIFT_MS(), ttlUpliftMs);
}

std::uint64_t MessagingSettings::getTtlUpliftMs() const
{
    return _settings.get<std::uint64_t>(SETTING_TTL_UPLIFT_MS());
}

std::int64_t MessagingSettings::getDiscoveryDefaultTimeoutMs() const
{
    return _settings.get<std::int64_t>(SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS());
}

void MessagingSettings::setDiscoveryDefaultTimeoutMs(std::int64_t discoveryDefaultTimeoutMs)
{
    _settings.set(SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS(), discoveryDefaultTimeoutMs);
}

std::int64_t MessagingSettings::getDiscoveryDefaultRetryIntervalMs() const
{
    return _settings.get<std::int64_t>(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS());
}

void MessagingSettings::setDiscoveryDefaultRetryIntervalMs(
        std::int64_t discoveryDefaultRetryIntervalMs)
{
    _settings.set(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS(), discoveryDefaultRetryIntervalMs);
}

std::int64_t MessagingSettings::getRoutingTableGracePeriodMs() const
{
    return _settings.get<std::int64_t>(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS());
}

void MessagingSettings::setRoutingTableGracePeriodMs(std::int64_t routingTableGracePeriodMs)
{
    _settings.set(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(), routingTableGracePeriodMs);
}

std::int64_t MessagingSettings::getRoutingTableCleanupIntervalMs() const
{
    return _settings.get<std::int64_t>(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS());
}

void MessagingSettings::setRoutingTableCleanupIntervalMs(std::int64_t routingTableCleanupIntervalMs)
{
    _settings.set(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(), routingTableCleanupIntervalMs);
}

bool MessagingSettings::getDiscardUnroutableRepliesAndPublications() const
{
    return _settings.get<bool>(SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS());
}

void MessagingSettings::setDiscardUnroutableRepliesAndPublications(
        const bool& discardUnRoutableRepliesAndPublications)
{
    _settings.set(SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS(),
                  discardUnRoutableRepliesAndPublications);
}

bool MessagingSettings::contains(const std::string& key) const
{
    return _settings.contains(key);
}

// Checks messaging settings and sets defaults
void MessagingSettings::checkSettings()
{
    assert(_settings.contains(SETTING_BROKER_URL()));
    std::string brokerUrl = _settings.get<std::string>(SETTING_BROKER_URL());
    if (brokerUrl.back() != '/') {
        brokerUrl.append("/");
        _settings.set(SETTING_BROKER_URL(), brokerUrl);
    }

    assert(_settings.contains(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));
    assert(_settings.contains(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    assert(_settings.contains(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    if (!_settings.contains(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS())) {
        _settings.set(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS(),
                      DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS().count());
    }
    if (!_settings.contains(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS())) {
        _settings.set(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS(),
                      DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS().count());
    }
    if (!_settings.contains(SETTING_MQTT_RECONNECT_MAX_DELAY())) {
        _settings.set(
                SETTING_MQTT_RECONNECT_MAX_DELAY(), getMqttReconnectDelayTimeSeconds().count());
    }
    if (!_settings.contains(SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED())) {
        _settings.set(SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED(), DEFAULT_MQTT_ENABLED());
    }
    if (!_settings.contains(SETTING_MQTT_CONNECTION_TIMEOUT_MS())) {
        _settings.set(
                SETTING_MQTT_CONNECTION_TIMEOUT_MS(), DEFAULT_MQTT_CONNECTION_TIMEOUT_MS().count());
    }
    if (!_settings.contains(SETTING_INDEX())) {
        _settings.set(SETTING_INDEX(), 0);
    }
    if (!_settings.contains(SETTING_SEND_MSG_RETRY_INTERVAL())) {
        _settings.set(SETTING_SEND_MSG_RETRY_INTERVAL(), 5000);
    }
    if (!_settings.contains(SETTING_PERSISTENCE_FILENAME())) {
        _settings.set(SETTING_PERSISTENCE_FILENAME(), DEFAULT_PERSISTENCE_FILENAME());
    }
    if (!_settings.contains(SETTING_DISCOVERY_MESSAGES_TTL_MS())) {
        _settings.set(SETTING_DISCOVERY_MESSAGES_TTL_MS(), DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS());
    }
    if (!_settings.contains(SETTING_MAXIMUM_TTL_MS())) {
        setMaximumTtlMs(DEFAULT_MAXIMUM_TTL_MS());
    }
    if (!_settings.contains(SETTING_TTL_UPLIFT_MS())) {
        _settings.set(SETTING_TTL_UPLIFT_MS(), DEFAULT_TTL_UPLIFT_MS());
    }

    if (!_settings.contains(SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS())) {
        _settings.set(
                SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS(), DEFAULT_DISCOVERY_DEFAULT_TIMEOUT_MS());
    }
    if (!_settings.contains(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS())) {
        _settings.set(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS(),
                      DEFAULT_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS());
    }
    if (!_settings.contains(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS())) {
        _settings.set(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS(),
                      DEFAULT_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS());
    }
    if (!_settings.contains(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS())) {
        _settings.set(
                SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(), DEFAULT_ROUTING_TABLE_GRACE_PERIOD_MS());
    }
    if (!_settings.contains(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS())) {
        _settings.set(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(),
                      DEFAULT_ROUTING_TABLE_CLEANUP_INTERVAL_MS());
    }
    if (!_settings.contains(SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS())) {
        _settings.set(SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS(),
                      DEFAULT_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS());
    }

    if (!checkMultipleBackendsSettings()) {
        const std::string message =
                "fatal failure in settings due to multiple backend configuration";
        throw joynr::exceptions::JoynrRuntimeException(message);
    }

    if (!_settings.contains(SETTING_GBID()) && !settingsContainMultipleBackendsConfiguration()) {
        _settings.set(SETTING_GBID(), DEFAULT_GBID());
    };
}

void MessagingSettings::checkAndSetDefaultMqttSettings(std::uint8_t index)
{
    if (!_settings.contains(SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index))) {
        _settings.set(SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index),
                      DEFAULT_MQTT_CONNECTION_TIMEOUT_MS().count());
        JOYNR_LOG_TRACE(logger(),
                        "backend index {}: Setting {} set to default {}",
                        index,
                        SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index),
                        _settings.get<std::string>(
                                SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index)));
    }
    if (!_settings.contains(SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index))) {
        _settings.set(SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index),
                      DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS().count());
        JOYNR_LOG_TRACE(logger(),
                        "backend index {}: Setting {} set to default {}",
                        index,
                        SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index),
                        _settings.get<std::string>(
                                SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index)));
    }
}

bool MessagingSettings::checkMultipleBackendsSettings()
{
    bool configurationCorrect = true;
    _additionalBackendsCount = 0;
    std::uint8_t& index = _additionalBackendsCount;
    for (;; index++) {
        if (_settings.contains(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)) &&
            !_settings.contains(SETTING_ADDITIONAL_BACKEND_GBID(index))) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "SETTING: {} = {} for index: {} is set but SETTING: {} = {} is "
                    "not",
                    SETTING_ADDITIONAL_BACKEND_BROKER_URL(index),
                    _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)),
                    index,
                    SETTING_ADDITIONAL_BACKEND_GBID(index),
                    _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_GBID(index)));

            configurationCorrect = false;
        } else if (!_settings.contains(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)) &&
                   _settings.contains(SETTING_ADDITIONAL_BACKEND_GBID(index))) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "SETTING: {} = {} for index: {} is set but SETTING: {} "
                    "= {} is not",
                    SETTING_ADDITIONAL_BACKEND_GBID(index),
                    _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_GBID(index)),
                    index,
                    SETTING_ADDITIONAL_BACKEND_BROKER_URL(index),
                    _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)));

            configurationCorrect = false;
        } else if (!_settings.contains(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)) &&
                   !_settings.contains(SETTING_ADDITIONAL_BACKEND_GBID(index))) {
            // all additional backends checked
            break;
        }

        checkAndSetDefaultMqttSettings(index);

        JOYNR_LOG_TRACE(logger(),
                        "Backend Index {}: {} = {}, {} = {}, {} = {}, {} = {}",
                        index,
                        SETTING_ADDITIONAL_BACKEND_GBID(index),
                        _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_GBID(index)),
                        SETTING_ADDITIONAL_BACKEND_BROKER_URL(index),
                        _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)),
                        SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index),
                        _settings.get<std::string>(
                                SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index)),
                        SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index),
                        _settings.get<std::string>(
                                SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index)));
    }
    if (settingsContainMultipleBackendsConfiguration() &&
        !(_settings.contains(SETTING_BROKER_URL()) && _settings.contains(SETTING_GBID()))) {

        JOYNR_LOG_ERROR(
                logger(),
                "Multiple Backends enabled, but default broker is not configured correctly: "
                "{} = {}, {} = {}",
                SETTING_GBID(),
                _settings.get<std::string>(SETTING_GBID()),
                SETTING_BROKER_URL(),
                _settings.get<std::string>(SETTING_BROKER_URL()));

        configurationCorrect = false;
    }

    return configurationCorrect;
}

bool MessagingSettings::settingsContainMultipleBackendsConfiguration() const
{
    return (_additionalBackendsCount > 0);
}

std::uint8_t MessagingSettings::getAdditionalBackendsCount() const
{
    return _additionalBackendsCount;
}

void MessagingSettings::printAdditionalBackendsSettings() const
{
    for (std::uint8_t index = 0; index < _additionalBackendsCount; index++) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_ADDITIONAL_BACKEND_GBID(index),
                       _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_GBID(index)));
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_ADDITIONAL_BACKEND_BROKER_URL(index),
                       _settings.get<std::string>(SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)));
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index),
                       _settings.get<std::string>(
                               SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index)));
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index),
                       _settings.get<std::string>(
                               SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index)));
    }
}

void MessagingSettings::printSettings() const
{
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_BROKER_URL(),
                   _settings.get<std::string>(SETTING_BROKER_URL()));

    if (_settings.contains(SETTING_GBID())) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_GBID(),
                       _settings.get<std::string>(SETTING_GBID()));
    }
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_DISCOVERY_DIRECTORIES_DOMAIN(),
                   _settings.get<std::string>(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CAPABILITIES_DIRECTORY_CHANNELID(),
                   _settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID(),
                   _settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS(),
                   _settings.get<std::string>(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS(),
                   _settings.get<std::string>(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MQTT_RECONNECT_MAX_DELAY(),
                   _settings.get<std::string>(SETTING_MQTT_RECONNECT_MAX_DELAY()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED(),
                   _settings.get<std::string>(SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_INDEX(),
                   _settings.get<std::string>(SETTING_INDEX()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_SEND_MSG_RETRY_INTERVAL(),
                   _settings.get<std::string>(SETTING_SEND_MSG_RETRY_INTERVAL()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_PERSISTENCE_FILENAME(),
                   _settings.get<std::string>(SETTING_PERSISTENCE_FILENAME()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_DISCOVERY_MESSAGES_TTL_MS(),
                   _settings.get<std::string>(SETTING_DISCOVERY_MESSAGES_TTL_MS()));
    JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_MAXIMUM_TTL_MS(), getMaximumTtlMs());
    JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_TTL_UPLIFT_MS(), getTtlUpliftMs());
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS(),
                   _settings.get<std::int64_t>(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(),
                   _settings.get<std::int64_t>(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(),
                   _settings.get<std::int64_t>(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS()));
    JOYNR_LOG_INFO(
            logger(),
            "SETTING: {} = {}",
            SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS(),
            _settings.get<std::string>(SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS()));
    printAdditionalBackendsSettings();
}

} // namespace joynr

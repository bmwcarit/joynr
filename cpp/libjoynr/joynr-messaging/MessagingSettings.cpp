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

namespace joynr
{

INIT_LOGGER(MessagingSettings);

MessagingSettings::MessagingSettings(Settings& settings) : settings(settings)
{
    settings.fillEmptySettingsWithDefaults(DEFAULT_MESSAGING_SETTINGS_FILENAME());
    checkSettings();
}

MessagingSettings::MessagingSettings(const MessagingSettings& other) : settings(other.settings)
{
    checkSettings();
}

const std::string& MessagingSettings::SETTING_BROKER_URL()
{
    static const std::string value("messaging/broker-url");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()
{
    static const std::string value("messaging/discovery-directories-domain");
    return value;
}

const std::string& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_URL()
{
    static const std::string value("messaging/capabilities-directory-url");
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

const std::string& MessagingSettings::SETTING_MQTT_CONNECTION_TIMEOUT_MS()
{
    static const std::string value("messaging/mqtt-connection-timeout-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_MQTT_MAX_MESSAGE_SIZE_BYTES()
{
    static const std::string value("messaging/mqtt-max-message-size-bytes");
    return value;
}

std::chrono::milliseconds MessagingSettings::DEFAULT_MQTT_CONNECTION_TIMEOUT_MS()
{
    static const std::chrono::milliseconds value(1000);
    return value;
}

std::int64_t MessagingSettings::DEFAULT_MQTT_MAX_MESSAGE_SIZE_BYTES()
{
    return MessagingSettings::NO_MQTT_MAX_MESSAGE_SIZE_BYTES();
}

std::int64_t MessagingSettings::NO_MQTT_MAX_MESSAGE_SIZE_BYTES()
{
    return 0;
}

std::chrono::seconds MessagingSettings::DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS()
{
    static const std::chrono::seconds value(1);
    return value;
}

const std::string& MessagingSettings::SETTING_INDEX()
{
    static const std::string value("messaging/index");
    return value;
}

const std::string& MessagingSettings::SETTING_CREATE_CHANNEL_RETRY_INTERVAL()
{
    static const std::string value("messaging/create-channel-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_DELETE_CHANNEL_RETRY_INTERVAL()
{
    static const std::string value("messaging/delete-channel-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_SEND_MSG_RETRY_INTERVAL()
{
    static const std::string value("messaging/send-msg-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_LONGPOLL_RETRY_INTERVAL()
{
    static const std::string value("messaging/longpoll-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS()
{
    static const std::string value("messaging/discovery-entry-expiry-interval-ms");
    return value;
}

const std::string& MessagingSettings::SETTING_LOCAL_PROXY_HOST()
{
    static const std::string value("messaging/local-proxy-host");
    return value;
}

const std::string& MessagingSettings::SETTING_HTTP_DEBUG()
{
    static const std::string value("messaging/http-debug");
    return value;
}

const std::string& MessagingSettings::SETTING_LOCAL_PROXY_PORT()
{
    static const std::string value("messaging/local-proxy-port");
    return value;
}

const std::string& MessagingSettings::SETTING_PERSISTENCE_FILENAME()
{
    static const std::string value("messaging/persistence-file");
    return value;
}

std::string MessagingSettings::getCertificateAuthority() const
{
    return settings.get<std::string>(SETTING_CERTIFICATE_AUTHORITY());
}

void MessagingSettings::setCertificateAuthority(const std::string& certificateAuthority)
{
    settings.set(SETTING_CERTIFICATE_AUTHORITY(), certificateAuthority);
}

std::string MessagingSettings::getClientCertificate() const
{
    return settings.get<std::string>(SETTING_CLIENT_CERTIFICATE());
}

void MessagingSettings::setClientCertificate(const std::string& clientCertificate)
{
    settings.set(SETTING_CLIENT_CERTIFICATE(), clientCertificate);
}

std::string MessagingSettings::getClientCertificatePassword() const
{
    return settings.get<std::string>(SETTING_CLIENT_CERTIFICATE_PASSWORD());
}

void MessagingSettings::setClientCertificatePassword(const std::string& clientCertificatePassword)
{
    settings.set(SETTING_CLIENT_CERTIFICATE_PASSWORD(), clientCertificatePassword);
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

const std::string& MessagingSettings::SETTING_LONGPOLL_TIMEOUT_MS()
{
    static const std::string value("messaging/long-poll-timeout");
    return value;
}

std::int64_t MessagingSettings::DEFAULT_LONGPOLL_TIMEOUT_MS()
{
    // 10 minutes
    return (10 * 60 * 100);
}

const std::string& MessagingSettings::SETTING_HTTP_CONNECT_TIMEOUT_MS()
{
    static const std::string value("messaging/http-connect-timeout");
    return value;
}

std::int64_t MessagingSettings::DEFAULT_HTTP_CONNECT_TIMEOUT_MS()
{
    // 1 minute
    return (1 * 60 * 1000);
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

const std::string& MessagingSettings::SETTING_TTL_UPLIFT_MS()
{
    static const std::string value("messaging/ttl-uplift-ms");
    return value;
}

BrokerUrl MessagingSettings::getBrokerUrl() const
{
    return BrokerUrl(settings.get<std::string>(SETTING_BROKER_URL()));
}

std::string MessagingSettings::getBrokerUrlString() const
{
    return settings.get<std::string>(SETTING_BROKER_URL());
}

void MessagingSettings::setBrokerUrl(const BrokerUrl& brokerUrl)
{
    std::string url = brokerUrl.getBrokerChannelsBaseUrl().toString();
    settings.set(SETTING_BROKER_URL(), url);
}

std::string MessagingSettings::getDiscoveryDirectoriesDomain() const
{
    return settings.get<std::string>(SETTING_DISCOVERY_DIRECTORIES_DOMAIN());
}

std::string MessagingSettings::getCapabilitiesDirectoryUrl() const
{
    return settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_URL());
}

std::string MessagingSettings::getCapabilitiesDirectoryChannelId() const
{
    return settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_CHANNELID());
}

std::string MessagingSettings::getCapabilitiesDirectoryParticipantId() const
{
    return settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID());
}

std::chrono::seconds MessagingSettings::getMqttKeepAliveTimeSeconds() const
{
    return std::chrono::seconds(settings.get<std::int64_t>(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()));
}

void MessagingSettings::setMqttKeepAliveTimeSeconds(std::chrono::seconds mqttKeepAliveTimeSeconds)
{
    settings.set(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS(), mqttKeepAliveTimeSeconds.count());
}

std::chrono::seconds MessagingSettings::getMqttReconnectDelayTimeSeconds() const
{
    return std::chrono::seconds(
            settings.get<std::int64_t>(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()));
}

void MessagingSettings::setMqttReconnectDelayTimeSeconds(
        std::chrono::seconds mqttReconnectDelayTimeSeconds)
{
    settings.set(
            SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS(), mqttReconnectDelayTimeSeconds.count());
}

std::chrono::milliseconds MessagingSettings::getMqttConnectionTimeoutMs() const
{
    return std::chrono::milliseconds(
            settings.get<std::int64_t>(SETTING_MQTT_CONNECTION_TIMEOUT_MS()));
}

std::int64_t MessagingSettings::getMqttMaxMessageSizeBytes() const
{
    return settings.get<std::int64_t>(SETTING_MQTT_MAX_MESSAGE_SIZE_BYTES());
}

void MessagingSettings::setMqttMaxMessageSizeBytes(std::int64_t mqttMaxMessageSizeBytes)
{
    settings.set(SETTING_MQTT_MAX_MESSAGE_SIZE_BYTES(), mqttMaxMessageSizeBytes);
}

std::int64_t MessagingSettings::getIndex() const
{
    return settings.get<std::int64_t>(SETTING_INDEX());
}

void MessagingSettings::setIndex(std::int64_t index)
{
    settings.set(SETTING_INDEX(), index);
}

int MessagingSettings::getCreateChannelRetryInterval() const
{
    return settings.get<int>(SETTING_CREATE_CHANNEL_RETRY_INTERVAL());
}

void MessagingSettings::setCreateChannelRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_CREATE_CHANNEL_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getDeleteChannelRetryInterval() const
{
    return settings.get<int>(SETTING_DELETE_CHANNEL_RETRY_INTERVAL());
}

void MessagingSettings::setDeleteChannelRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_DELETE_CHANNEL_RETRY_INTERVAL(), retryInterval);
}

std::int64_t MessagingSettings::getDiscoveryEntryExpiryIntervalMs() const
{
    return settings.get<std::int64_t>(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS());
}

void MessagingSettings::setDiscoveryEntryExpiryIntervalMs(std::int64_t expiryIntervalMs)
{
    settings.set(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS(), expiryIntervalMs);
}

int MessagingSettings::getSendMsgRetryInterval() const
{
    return settings.get<int>(SETTING_SEND_MSG_RETRY_INTERVAL());
}

void MessagingSettings::setSendMsgRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_SEND_MSG_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getLongPollRetryInterval() const
{
    return settings.get<int>(SETTING_LONGPOLL_RETRY_INTERVAL());
}

void MessagingSettings::setLongPollRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_LONGPOLL_RETRY_INTERVAL(), retryInterval);
}

std::string MessagingSettings::getLocalProxyPort() const
{
    return settings.get<std::string>(SETTING_LOCAL_PROXY_PORT());
}

void MessagingSettings::setLocalProxyPort(const int& localProxyPort)
{
    settings.set(SETTING_LOCAL_PROXY_PORT(), localProxyPort);
}

std::string MessagingSettings::getLocalProxyHost() const
{
    return settings.get<std::string>(SETTING_LOCAL_PROXY_HOST());
}

void MessagingSettings::setLocalProxyHost(const std::string& localProxyHost)
{
    settings.set(SETTING_LOCAL_PROXY_HOST(), localProxyHost);
}

bool MessagingSettings::getHttpDebug() const
{
    return settings.get<bool>(SETTING_HTTP_DEBUG());
}

void MessagingSettings::setHttpDebug(const bool& httpDebug)
{
    settings.set(SETTING_HTTP_DEBUG(), httpDebug);
}

std::string MessagingSettings::getMessagingPropertiesPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_PERSISTENCE_FILENAME());
}

void MessagingSettings::setMessagingPropertiesPersistenceFilename(const std::string& filename)
{
    settings.set(SETTING_PERSISTENCE_FILENAME(), filename);
}

std::int64_t MessagingSettings::getLongPollTimeoutMs() const
{
    return settings.get<std::int64_t>(SETTING_LONGPOLL_TIMEOUT_MS());
}

void MessagingSettings::setLongPollTimeoutMs(std::int64_t timeout_ms)
{
    settings.set(SETTING_LONGPOLL_TIMEOUT_MS(), timeout_ms);
}

std::int64_t MessagingSettings::getHttpConnectTimeout() const
{
    return settings.get<std::int64_t>(SETTING_HTTP_CONNECT_TIMEOUT_MS());
}

void MessagingSettings::setHttpConnectTimeoutMs(std::int64_t timeout_ms)
{
    settings.set(SETTING_HTTP_CONNECT_TIMEOUT_MS(), timeout_ms);
}

std::int64_t MessagingSettings::getBrokerTimeoutMs() const
{
    return settings.get<std::int64_t>(SETTING_BROKER_TIMEOUT_MS());
}

void MessagingSettings::setBrokerTimeoutMs(std::int64_t timeout_ms)
{
    settings.set(SETTING_BROKER_TIMEOUT_MS(), timeout_ms);
}

std::uint64_t MessagingSettings::getMaximumTtlMs() const
{
    return settings.get<std::uint64_t>(SETTING_MAXIMUM_TTL_MS());
}

void MessagingSettings::setMaximumTtlMs(std::uint64_t maximumTtlMs)
{
    settings.set(SETTING_MAXIMUM_TTL_MS(), maximumTtlMs);
}

std::int64_t MessagingSettings::getDiscoveryMessagesTtl() const
{
    return settings.get<std::int64_t>(SETTING_DISCOVERY_MESSAGES_TTL_MS());
}

void MessagingSettings::setDiscoveryMessagesTtl(std::int64_t ttl_ms)
{
    settings.set(SETTING_DISCOVERY_MESSAGES_TTL_MS(), ttl_ms);
}

std::int64_t MessagingSettings::getSendMsgMaxTtl() const
{
    return settings.get<std::int64_t>(SETTING_SEND_MESSAGE_MAX_TTL());
}

void MessagingSettings::setSendMsgMaxTtl(std::int64_t ttl_ms)
{
    settings.set(SETTING_SEND_MESSAGE_MAX_TTL(), ttl_ms);
}

void MessagingSettings::setTtlUpliftMs(std::uint64_t ttlUpliftMs)
{
    settings.set(SETTING_TTL_UPLIFT_MS(), ttlUpliftMs);
}

std::uint64_t MessagingSettings::getTtlUpliftMs() const
{
    return settings.get<std::uint64_t>(SETTING_TTL_UPLIFT_MS());
}

std::int64_t MessagingSettings::getDiscoveryDefaultTimeoutMs() const
{
    return settings.get<std::int64_t>(SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS());
}

void MessagingSettings::setDiscoveryDefaultTimeoutMs(std::int64_t discoveryDefaultTimeoutMs)
{
    settings.set(SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS(), discoveryDefaultTimeoutMs);
}

std::int64_t MessagingSettings::getDiscoveryDefaultRetryIntervalMs() const
{
    return settings.get<std::int64_t>(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS());
}

void MessagingSettings::setDiscoveryDefaultRetryIntervalMs(
        std::int64_t discoveryDefaultRetryIntervalMs)
{
    settings.set(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS(), discoveryDefaultRetryIntervalMs);
}

std::int64_t MessagingSettings::getRoutingTableGracePeriodMs() const
{
    return settings.get<std::int64_t>(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS());
}

void MessagingSettings::setRoutingTableGracePeriodMs(std::int64_t routingTableGracePeriodMs)
{
    settings.set(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(), routingTableGracePeriodMs);
}

std::int64_t MessagingSettings::getRoutingTableCleanupIntervalMs() const
{
    return settings.get<std::int64_t>(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS());
}

void MessagingSettings::setRoutingTableCleanupIntervalMs(std::int64_t routingTableCleanupIntervalMs)
{
    settings.set(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(), routingTableCleanupIntervalMs);
}

bool MessagingSettings::contains(const std::string& key) const
{
    return settings.contains(key);
}

// Checks messaging settings and sets defaults
void MessagingSettings::checkSettings()
{
    assert(settings.contains(SETTING_BROKER_URL()));
    std::string brokerUrl = settings.get<std::string>(SETTING_BROKER_URL());
    if (brokerUrl.back() != '/') {
        brokerUrl.append("/");
        settings.set(SETTING_BROKER_URL(), brokerUrl);
    }

    assert(settings.contains(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_URL()));
    std::string capabilitiesDirectoryUrl =
            settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_URL());
    if (capabilitiesDirectoryUrl.back() != '/') {
        capabilitiesDirectoryUrl.append("/");
        settings.set(SETTING_CAPABILITIES_DIRECTORY_URL(), capabilitiesDirectoryUrl);
    }
    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    if (!settings.contains(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS())) {
        settings.set(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS(),
                     DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS().count());
    }
    if (!settings.contains(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS())) {
        settings.set(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS(),
                     DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS().count());
    }
    if (!settings.contains(SETTING_MQTT_CONNECTION_TIMEOUT_MS())) {
        settings.set(
                SETTING_MQTT_CONNECTION_TIMEOUT_MS(), DEFAULT_MQTT_CONNECTION_TIMEOUT_MS().count());
    }
    if (!settings.contains(SETTING_MQTT_MAX_MESSAGE_SIZE_BYTES())) {
        settings.set(SETTING_MQTT_MAX_MESSAGE_SIZE_BYTES(), DEFAULT_MQTT_MAX_MESSAGE_SIZE_BYTES());
    }
    if (!settings.contains(SETTING_INDEX())) {
        settings.set(SETTING_INDEX(), 0);
    }
    if (!settings.contains(SETTING_CREATE_CHANNEL_RETRY_INTERVAL())) {
        settings.set(SETTING_CREATE_CHANNEL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_DELETE_CHANNEL_RETRY_INTERVAL())) {
        settings.set(SETTING_DELETE_CHANNEL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_SEND_MSG_RETRY_INTERVAL())) {
        settings.set(SETTING_SEND_MSG_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_LONGPOLL_RETRY_INTERVAL())) {
        settings.set(SETTING_LONGPOLL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_PERSISTENCE_FILENAME())) {
        settings.set(SETTING_PERSISTENCE_FILENAME(), DEFAULT_PERSISTENCE_FILENAME());
    }
    if (!settings.contains(SETTING_DISCOVERY_MESSAGES_TTL_MS())) {
        settings.set(SETTING_DISCOVERY_MESSAGES_TTL_MS(), DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS());
    }
    if (!settings.contains(SETTING_MAXIMUM_TTL_MS())) {
        setMaximumTtlMs(DEFAULT_MAXIMUM_TTL_MS());
    }
    if (!settings.contains(SETTING_TTL_UPLIFT_MS())) {
        settings.set(SETTING_TTL_UPLIFT_MS(), DEFAULT_TTL_UPLIFT_MS());
    }

    if (!settings.contains(SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS())) {
        settings.set(
                SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS(), DEFAULT_DISCOVERY_DEFAULT_TIMEOUT_MS());
    }
    if (!settings.contains(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS())) {
        settings.set(SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS(),
                     DEFAULT_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS());
    }
    if (!settings.contains(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS())) {
        settings.set(SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS(),
                     DEFAULT_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS());
    }
    if (!settings.contains(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS())) {
        settings.set(
                SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(), DEFAULT_ROUTING_TABLE_GRACE_PERIOD_MS());
    }
    if (!settings.contains(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS())) {
        settings.set(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(),
                     DEFAULT_ROUTING_TABLE_CLEANUP_INTERVAL_MS());
    }
}

void MessagingSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {} = {})",
                    SETTING_BROKER_URL(),
                    settings.get<std::string>(SETTING_BROKER_URL()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {} = {})",
                    SETTING_DISCOVERY_DIRECTORIES_DOMAIN(),
                    settings.get<std::string>(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_CAPABILITIES_DIRECTORY_URL(),
                    settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_URL()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_CAPABILITIES_DIRECTORY_CHANNELID(),
                    settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID(),
                    settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS(),
                    settings.get<std::string>(SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS(),
                    settings.get<std::string>(SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_INDEX(),
                    settings.get<std::string>(SETTING_INDEX()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_CREATE_CHANNEL_RETRY_INTERVAL(),
                    settings.get<std::string>(SETTING_CREATE_CHANNEL_RETRY_INTERVAL()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_DELETE_CHANNEL_RETRY_INTERVAL(),
                    settings.get<std::string>(SETTING_DELETE_CHANNEL_RETRY_INTERVAL()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_SEND_MSG_RETRY_INTERVAL(),
                    settings.get<std::string>(SETTING_SEND_MSG_RETRY_INTERVAL()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_LONGPOLL_RETRY_INTERVAL(),
                    settings.get<std::string>(SETTING_LONGPOLL_RETRY_INTERVAL()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_LOCAL_PROXY_HOST(),
                    settings.get<std::string>(SETTING_LOCAL_PROXY_HOST()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_LOCAL_PROXY_PORT(),
                    settings.get<std::string>(SETTING_LOCAL_PROXY_PORT()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_PERSISTENCE_FILENAME(),
                    settings.get<std::string>(SETTING_PERSISTENCE_FILENAME()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_DISCOVERY_MESSAGES_TTL_MS(),
                    settings.get<std::string>(SETTING_DISCOVERY_MESSAGES_TTL_MS()));
    JOYNR_LOG_DEBUG(logger, "SETTING: {} = {})", SETTING_MAXIMUM_TTL_MS(), getMaximumTtlMs());
    JOYNR_LOG_DEBUG(logger, "SETTING: {} = {})", SETTING_TTL_UPLIFT_MS(), getTtlUpliftMs());
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {} = {})",
                    SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(),
                    settings.get<std::int64_t>(SETTING_ROUTING_TABLE_GRACE_PERIOD_MS()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {} = {})",
                    SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(),
                    settings.get<std::int64_t>(SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS()));
}

} // namespace joynr

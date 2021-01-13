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
#ifndef MESSAGINGSETTINGS_H
#define MESSAGINGSETTINGS_H

#include <chrono>
#include <cstdint>
#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

namespace joynr
{

class BrokerUrl;
class Settings;

class JOYNR_EXPORT MessagingSettings
{
public:
    explicit MessagingSettings(Settings& settings);
    MessagingSettings(const MessagingSettings& other);

    ~MessagingSettings() = default;

    static const std::string& SETTING_BROKER_URL();
    static const std::string& SETTING_GBID();
    static std::string SETTING_ADDITIONAL_BACKEND_BROKER_URL(std::uint8_t index);
    static std::string SETTING_ADDITIONAL_BACKEND_GBID(std::uint8_t index);
    static std::string SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(std::uint8_t index);
    static std::string SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(std::uint8_t index);

    static const std::string& SETTING_DISCOVERY_DIRECTORIES_DOMAIN();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_CHANNELID();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID();
    static const std::string& SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS();
    static const std::string& SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS();
    static const std::string& SETTING_MQTT_RECONNECT_MAX_DELAY();
    static const std::string& SETTING_MQTT_EXPONENTIAL_BACKOFF_ENABLED();
    static const std::string& SETTING_MQTT_CONNECTION_TIMEOUT_MS();
    static const std::string& SETTING_MQTT_MAX_MESSAGE_SIZE_BYTES();
    static const std::string& SETTING_INDEX();
    static const std::string& SETTING_SEND_MSG_RETRY_INTERVAL();
    static const std::string& SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS();

    static const std::string& SETTING_CERTIFICATE_AUTHORITY();
    static const std::string& SETTING_CLIENT_CERTIFICATE();
    static const std::string& SETTING_CLIENT_CERTIFICATE_PASSWORD();

    static const std::string& SETTING_PERSISTENCE_FILENAME();
    static const std::string& SETTING_BROKER_TIMEOUT_MS();

    static const std::string& SETTING_DISCOVERY_DEFAULT_TIMEOUT_MS();
    static const std::string& SETTING_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS();

    static const std::string& SETTING_ROUTING_TABLE_GRACE_PERIOD_MS();
    static const std::string& SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS();

    static const std::string& SETTING_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS();

    /**
     * @brief SETTING_MAXIMUM_TTL_MS The key used in settings to identifiy the maximum allowed value
     * of the time-to-live joynr message header.
     *
     * @return the key used in settings for the maximum TTL message value.
     */
    static const std::string& SETTING_MAXIMUM_TTL_MS();
    /**
     * @brief SETTING_DISCOVERY_MESSAGES_TTL_MS Time-to-live of messages used
     * in communication between the local discovery service and the discovery
     * backend service.
     *
     * @return the TTL used for discovery messages send to the backend
     * discovery service.
     */
    static const std::string& SETTING_DISCOVERY_MESSAGES_TTL_MS();
    static const std::string& SETTING_SEND_MESSAGE_MAX_TTL();
    static const std::string& SETTING_TTL_UPLIFT_MS();

    static const std::string& DEFAULT_GBID();
    static const std::string& DEFAULT_MESSAGING_SETTINGS_FILENAME();
    static const std::string& DEFAULT_PERSISTENCE_FILENAME();
    static std::int64_t DEFAULT_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS();
    static std::int64_t DEFAULT_BROKER_TIMEOUT_MS();
    static std::int64_t DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS();
    static std::int64_t DEFAULT_DISCOVERY_DEFAULT_TIMEOUT_MS();
    static std::int64_t DEFAULT_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS();
    static std::int64_t DEFAULT_ROUTING_TABLE_GRACE_PERIOD_MS();
    static std::int64_t DEFAULT_ROUTING_TABLE_CLEANUP_INTERVAL_MS();
    static std::int64_t DEFAULT_SEND_MESSAGE_MAX_TTL();
    static std::uint64_t DEFAULT_TTL_UPLIFT_MS();
    static bool DEFAULT_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS();

    /**
     * @brief DEFAULT_MAXIMUM_TTL_MS
     * @return the default value for maximum message TTL in milliseconds.
     * @see SETTING_MAXIMUM_TTL_MS
     */
    static std::uint64_t DEFAULT_MAXIMUM_TTL_MS();
    static std::chrono::seconds DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS();
    static std::chrono::seconds DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS();
    static bool DEFAULT_MQTT_ENABLED();
    static std::chrono::milliseconds DEFAULT_MQTT_CONNECTION_TIMEOUT_MS();

    BrokerUrl getBrokerUrl() const;
    std::string getBrokerUrlString() const;
    void setBrokerUrl(const BrokerUrl& brokerUrl);

    std::string getGbid() const;
    void setGbid(const std::string& gbid);

    BrokerUrl getAdditionalBackendBrokerUrl(std::uint8_t index) const;
    std::string getAdditionalBackendBrokerUrlString(std::uint8_t index) const;
    void setAdditionalBackendBrokerUrl(const BrokerUrl& brokerUrl, std::uint8_t index);
    std::string getAdditionalBackendGbid(std::uint8_t index) const;
    void setAdditionalBackendGbid(const std::string& gbid, std::uint8_t index);
    std::chrono::seconds getAdditionalBackendMqttKeepAliveTimeSeconds(std::uint8_t index) const;
    void setAdditionalBackendMqttKeepAliveTimeSeconds(std::chrono::seconds mqttKeepAliveTimeSeconds,
                                                      std::uint8_t index);
    std::chrono::milliseconds getAdditionalBackendMqttConnectionTimeoutMs(std::uint8_t index) const;

    std::string getDiscoveryDirectoriesDomain() const;

    std::string getCapabilitiesDirectoryChannelId() const;
    std::string getCapabilitiesDirectoryParticipantId() const;

    std::chrono::seconds getMqttKeepAliveTimeSeconds() const;
    void setMqttKeepAliveTimeSeconds(std::chrono::seconds mqttKeepAliveTimeSeconds);
    std::chrono::seconds getMqttReconnectDelayTimeSeconds() const;
    void setMqttReconnectDelayTimeSeconds(std::chrono::seconds mqttReconnectDelayTimeSeconds);
    std::chrono::seconds getMqttReconnectMaxDelayTimeSeconds() const;
    void setMqttReconnectMaxDelayTimeSeconds(std::chrono::seconds mqttReconnectMaxDelayTimeSeconds);
    bool getMqttExponentialBackoffEnabled() const;
    void setMqttExponentialBackoffEnabled(const bool& enable);
    std::chrono::milliseconds getMqttConnectionTimeoutMs() const;
    void setIndex(std::int64_t index);
    std::int64_t getIndex() const;
    std::int64_t getDiscoveryEntryExpiryIntervalMs() const;
    void setDiscoveryEntryExpiryIntervalMs(std::int64_t expiryIntervalMs);
    std::uint32_t getSendMsgRetryInterval() const;
    void setSendMsgRetryInterval(const std::uint32_t& retryInterval);
    std::string getCertificateAuthority() const;
    void setCertificateAuthority(const std::string& certificateAuthority);
    std::string getClientCertificate() const;
    void setClientCertificate(const std::string& clientCertificate);
    std::string getClientCertificatePassword() const;
    void setClientCertificatePassword(const std::string& clientCertificatePassword);
    std::string getMessagingPropertiesPersistenceFilename() const;
    void setMessagingPropertiesPersistenceFilename(const std::string& persistenceFilename);
    std::int64_t getBrokerTimeoutMs() const;
    void setBrokerTimeoutMs(std::int64_t timeout_ms);
    std::int64_t getRoutingTableGracePeriodMs() const;
    void setRoutingTableGracePeriodMs(std::int64_t routingTableGracePeriodMs);
    std::int64_t getRoutingTableCleanupIntervalMs() const;
    void setRoutingTableCleanupIntervalMs(std::int64_t routingTableCleanupIntervalMs);

    /**
     * @brief getMaximumTtlMs Get the maximum allowed time-to-live value in milliseconds for joynr
     * messages.
     *
     * @return the maximum TTL for joynr messages.
     */
    std::uint64_t getMaximumTtlMs() const;
    /**
     * @brief setMaximumTtlMs Set the maximum allowed time-to-live value in milliseconds for joynr
     * messages.
     *
     * @param maximumTtlMs the new maximum TTL for joynr messages.
     */
    void setMaximumTtlMs(std::uint64_t maximumTtlMs);
    std::int64_t getDiscoveryMessagesTtl() const;
    void setDiscoveryMessagesTtl(std::int64_t ttl_ms);
    std::int64_t getSendMsgMaxTtl() const;
    void setSendMsgMaxTtl(std::int64_t ttl_ms);
    void setTtlUpliftMs(std::uint64_t ttlUpliftMs);
    std::uint64_t getTtlUpliftMs() const;

    std::int64_t getDiscoveryDefaultTimeoutMs() const;
    void setDiscoveryDefaultTimeoutMs(std::int64_t discoveryDefaultTimeoutMs);
    std::int64_t getDiscoveryDefaultRetryIntervalMs() const;
    void setDiscoveryDefaultRetryIntervalMs(std::int64_t discoveryDefaultRetryIntervalMs);

    bool getDiscardUnroutableRepliesAndPublications() const;
    void setDiscardUnroutableRepliesAndPublications(
            const bool& discardUnroutableRepliesAndPublications);

    bool contains(const std::string& key) const;

    bool settingsContainMultipleBackendsConfiguration() const;

    void printSettings() const;
    void printAdditionalBackendsSettings() const;
    std::uint8_t getAdditionalBackendsCount() const;

private:
    void operator=(const MessagingSettings& other);
    std::uint8_t _additionalBackendsCount;
    Settings& _settings;
    ADD_LOGGER(MessagingSettings)
    void checkSettings();
    bool checkMultipleBackendsSettings();
    void checkAndSetDefaultMqttSettings(std::uint8_t index);
};

} // namespace joynr
#endif // MESSAGINGSETTINGS_H

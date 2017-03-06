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
    static const std::string& SETTING_DISCOVERY_DIRECTORIES_DOMAIN();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_URL();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_CHANNELID();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID();
    static const std::string& SETTING_MQTT_KEEP_ALIVE_TIME();
    static const std::string& SETTING_MQTT_RECONNECT_SLEEP_TIME();
    static const std::string& SETTING_INDEX();
    static const std::string& SETTING_CREATE_CHANNEL_RETRY_INTERVAL();
    static const std::string& SETTING_DELETE_CHANNEL_RETRY_INTERVAL();
    static const std::string& SETTING_SEND_MSG_RETRY_INTERVAL();
    static const std::string& SETTING_LONGPOLL_RETRY_INTERVAL();
    static const std::string& SETTING_DISCOVERY_ENTRY_EXPIRY_INTERVAL_MS();
    static const std::string& SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS();

    static const std::string& SETTING_LOCAL_PROXY_HOST();
    static const std::string& SETTING_LOCAL_PROXY_PORT();

    static const std::string& SETTING_CERTIFICATE_AUTHORITY();
    static const std::string& SETTING_CLIENT_CERTIFICATE();
    static const std::string& SETTING_CLIENT_CERTIFICATE_PASSWORD();

    static const std::string& SETTING_HTTP_DEBUG();
    static const std::string& SETTING_PERSISTENCE_FILENAME();
    static const std::string& SETTING_LONGPOLL_TIMEOUT_MS();
    static const std::string& SETTING_HTTP_CONNECT_TIMEOUT_MS();
    static const std::string& SETTING_BROKER_TIMEOUT_MS();

    static const std::string& ACCESS_CONTROL_ENABLE();

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
    static const std::string& SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS();
    static const std::string& SETTING_TTL_UPLIFT_MS();

    static std::chrono::milliseconds DEFAULT_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS();
    static const std::string& DEFAULT_MESSAGING_SETTINGS_FILENAME();
    static const std::string& DEFAULT_PERSISTENCE_FILENAME();
    static std::int64_t DEFAULT_LONGPOLL_TIMEOUT_MS();
    static std::int64_t DEFAULT_HTTP_CONNECT_TIMEOUT_MS();
    static std::int64_t DEFAULT_BROKER_TIMEOUT_MS();
    static std::int64_t DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS();
    static std::int64_t DEFAULT_SEND_MESSAGE_MAX_TTL();
    static std::uint64_t DEFAULT_TTL_UPLIFT_MS();
    /**
     * @brief DEFAULT_MAXIMUM_TTL_MS
     * @return the default value for maximum message TTL in milliseconds.
     * @see SETTING_MAXIMUM_TTL_MS
     */
    static std::uint64_t DEFAULT_MAXIMUM_TTL_MS();
    static std::chrono::seconds DEFAULT_MQTT_KEEP_ALIVE_TIME();
    static std::chrono::milliseconds DEFAULT_MQTT_RECONNECT_SLEEP_TIME();
    static int DEFAULT_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS();

    static bool DEFAULT_ENABLE_ACCESS_CONTROLLER();

    BrokerUrl getBrokerUrl() const;
    std::string getBrokerUrlString() const;
    void setBrokerUrl(const BrokerUrl& brokerUrl);

    std::string getDiscoveryDirectoriesDomain() const;

    std::string getCapabilitiesDirectoryUrl() const;
    std::string getCapabilitiesDirectoryChannelId() const;
    std::string getCapabilitiesDirectoryParticipantId() const;

    std::chrono::seconds getMqttKeepAliveTime() const;
    void setMqttKeepAliveTime(std::chrono::seconds mqttKeepAliveTime);
    std::chrono::milliseconds getMqttReconnectSleepTime() const;
    void setMqttReconnectSleepTime(std::chrono::milliseconds mqttReconnectSleepTime);
    std::int64_t getIndex() const;
    void setIndex(std::int64_t index);
    int getCreateChannelRetryInterval() const;
    void setCreateChannelRetryInterval(const int& retryInterval);
    int getDeleteChannelRetryInterval() const;
    void setDeleteChannelRetryInterval(const int& retryInterval);
    int getDiscoveryEntryExpiryIntervalMs() const;
    void setDiscoveryEntryExpiryIntervalMs(int expiryIntervalMs);
    int getPurgeExpiredDiscoveryEntriesIntervalMs() const;
    void setPurgeExpiredDiscoveryEntriesIntervalMs(int purgeExpiredEntriesIntervalMs);
    int getSendMsgRetryInterval() const;
    void setSendMsgRetryInterval(const int& retryInterval);
    int getLongPollRetryInterval() const;
    void setLongPollRetryInterval(const int& retryInterval);
    std::string getLocalProxyHost() const;
    void setLocalProxyHost(const std::string& localProxyHost);
    std::string getLocalProxyPort() const;
    void setLocalProxyPort(const int& localProxyPort);
    void setHttpDebug(const bool& httpDebug);
    bool getHttpDebug() const;
    std::string getCertificateAuthority() const;
    void setCertificateAuthority(const std::string& certificateAuthority);
    std::string getClientCertificate() const;
    void setClientCertificate(const std::string& clientCertificate);
    std::string getClientCertificatePassword() const;
    void setClientCertificatePassword(const std::string& clientCertificatePassword);
    std::string getMessagingPropertiesPersistenceFilename() const;
    void setMessagingPropertiesPersistenceFilename(const std::string& persistenceFilename);
    std::int64_t getLongPollTimeout() const;
    void setLongPollTimeout(std::int64_t timeout_ms);
    std::int64_t getHttpConnectTimeout() const;
    void setHttpConnectTimeout(std::int64_t timeout_ms);
    std::int64_t getBrokerTimeout() const;
    void setBrokerTimeout(std::int64_t timeout_ms);

    bool enableAccessController() const;
    void setEnableAccessController(bool enable);

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

    std::chrono::milliseconds getCapabilitiesFreshnessUpdateIntervalMs() const;

    bool contains(const std::string& key) const;

    void printSettings() const;

private:
    void operator=(const MessagingSettings& other);

    Settings& settings;
    ADD_LOGGER(MessagingSettings);
    void checkSettings();
};

} // namespace joynr
#endif // MESSAGINGSETTINGS_H

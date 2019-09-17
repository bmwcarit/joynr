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
#ifndef CLUSTERCONTROLLERSETTINGS_H
#define CLUSTERCONTROLLERSETTINGS_H

#include <chrono>
#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

namespace joynr
{

class Settings;

class JOYNR_EXPORT ClusterControllerSettings
{
public:
    static const std::string& SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS();
    static const std::string& SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME();
    static const std::string& SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED();
    static const std::string& SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME();
    static const std::string& SETTING_MESSAGE_QUEUE_LIMIT();
    static const std::string& SETTING_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT();
    static const std::string& SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT();
    static const std::string& SETTING_MESSAGE_QUEUE_LIMIT_BYTES();
    static const std::string& SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES();
    static const std::string& SETTING_MQTT_CLIENT_ID_PREFIX();
    static const std::string& SETTING_MQTT_TLS_ENABLED();
    static const std::string& SETTING_MQTT_TLS_VERSION();
    static const std::string& SETTING_MQTT_TLS_CIPHERS();
    static const std::string& SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME();
    static const std::string& SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH();
    static const std::string& SETTING_MQTT_CERTIFICATE_PEM_FILENAME();
    static const std::string& SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME();
    static const std::string& SETTING_MQTT_USERNAME();
    static const std::string& SETTING_MQTT_PASSWORD();
    static const std::string& SETTING_MQTT_MULTICAST_TOPIC_PREFIX();
    static const std::string& SETTING_MQTT_UNICAST_TOPIC_PREFIX();
    static const std::string& SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME();
    static const std::string& SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED();
    static const std::string& SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS();
    static const std::string& SETTING_WS_TLS_PORT();
    static const std::string& SETTING_WS_PORT();
    static const std::string& SETTING_USE_ONLY_LDAS();
    static const std::string& SETTING_ACCESS_CONTROL_AUDIT();

    static const std::string& SETTING_ACCESS_CONTROL_ENABLE();
    static const std::string& SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS();
    static const std::string&
    SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID();
    static const std::string& SETTING_ACL_ENTRIES_DIRECTORY();
    static const std::string& SETTING_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED();

    static std::chrono::milliseconds DEFAULT_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS();
    static const std::string& DEFAULT_CLUSTERCONTROLLER_SETTINGS_FILENAME();
    static const std::string& DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME();
    static bool DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED();
    static const std::string& DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME();
    static const std::string& DEFAULT_MQTT_CLIENT_ID_PREFIX();
    static bool DEFAULT_MQTT_TLS_ENABLED();
    static const std::string& DEFAULT_MQTT_TLS_VERSION();
    static const std::string& DEFAULT_MQTT_TLS_CIPHERS();
    static const std::string& DEFAULT_MQTT_MULTICAST_TOPIC_PREFIX();
    static const std::string& DEFAULT_MQTT_UNICAST_TOPIC_PREFIX();
    static const std::string& DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME();
    static bool DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED();
    static int DEFAULT_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS();
    static bool DEFAULT_ENABLE_ACCESS_CONTROLLER();
    static bool DEFAULT_USE_ONLY_LDAS();
    static bool DEFAULT_ACCESS_CONTROL_AUDIT();
    static std::uint64_t DEFAULT_MESSAGE_QUEUE_LIMIT();
    static std::uint64_t DEFAULT_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT();
    static std::uint64_t DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT();
    static std::uint64_t DEFAULT_MESSAGE_QUEUE_LIMIT_BYTES();
    static std::uint64_t DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES();
    static bool DEFAULT_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED();

    explicit ClusterControllerSettings(Settings& settings);
    ClusterControllerSettings(const ClusterControllerSettings&) = default;
    ClusterControllerSettings(ClusterControllerSettings&&) = default;

    ~ClusterControllerSettings() = default;

    std::string getMulticastReceiverDirectoryPersistenceFilename() const;
    void setMulticastReceiverDirectoryPersistenceFilename(const std::string& filename);
    bool isMulticastReceiverDirectoryPersistencyEnabled() const;
    void setMulticastReceiverDirectoryPersistencyEnabled(bool enabled);

    bool isWsTLSPortSet() const;
    std::uint16_t getWsTLSPort() const;
    void setWsTLSPort(std::uint16_t port);

    bool isWsPortSet() const;
    std::uint16_t getWsPort() const;
    void setWsPort(std::uint16_t port);

    bool isMqttClientIdPrefixSet() const;
    std::string getMqttClientIdPrefix() const;
    void setMqttClientIdPrefix(const std::string& mqttClientId);

    std::string getMqttMulticastTopicPrefix() const;
    void setMqttMulticastTopicPrefix(const std::string& mqttMulticastTopicPrefix);

    std::string getMqttUnicastTopicPrefix() const;
    void setMqttUnicastTopicPrefix(const std::string& mqttUnicastTopicPrefix);

    bool isMqttCertificateAuthorityPemFilenameSet() const;
    std::string getMqttCertificateAuthorityPemFilename() const;

    bool isMqttCertificateAuthorityCertificateFolderPathSet() const;
    std::string getMqttCertificateAuthorityCertificateFolderPath() const;

    bool isMqttCertificatePemFilenameSet() const;
    std::string getMqttCertificatePemFilename() const;

    bool isMqttPrivateKeyPemFilenameSet() const;
    std::string getMqttPrivateKeyPemFilename() const;

    bool isMqttUsernameSet() const;
    std::string getMqttUsername() const;

    bool isMqttPasswordSet() const;
    std::string getMqttPassword() const;

    void setMqttTlsEnabled(bool enabled);
    bool isMqttTlsEnabled() const;

    void setMqttTlsVersion(const std::string& tlsVersion);
    std::string getMqttTlsVersion() const;

    void setMqttTlsCiphers(const std::string& tlsCiphers);
    std::string getMqttTlsCiphers() const;

    std::string getLocalDomainAccessStorePersistenceFilename() const;
    void setLocalDomainAccessStorePersistenceFilename(const std::string& filename);

    std::uint64_t getMessageQueueLimit() const;
    void setMessageQueueLimit(std::uint64_t limit);

    std::uint64_t getPerParticipantIdMessageQueueLimit() const;
    void setPerParticipantIdMessageQueueLimit(std::uint64_t limit);

    std::uint64_t getTransportNotAvailableQueueLimit() const;
    void setTransportNotAvailableQueueLimit(std::uint64_t limit);

    std::uint64_t getMessageQueueLimitBytes() const;
    void setMessageQueueLimitBytes(std::uint64_t limitBytes);

    std::uint64_t getTransportNotAvailableQueueLimitBytes() const;
    void setTransportNotAvailableQueueLimitBytes(std::uint64_t limitBytes);

    bool enableAccessController() const;
    void setEnableAccessController(bool enable);

    bool aclAudit() const;
    void setAclAudit(bool enable);

    std::string getGlobalDomainAccessControlAddress() const;
    std::string getGlobalDomainAccessControlParticipantId() const;

    bool getUseOnlyLDAS() const;
    void setUseOnlyLDAS(bool useLDASonly);

    std::string getLocalCapabilitiesDirectoryPersistenceFilename() const;
    void setLocalCapabilitiesDirectoryPersistenceFilename(const std::string& filename);
    bool isLocalCapabilitiesDirectoryPersistencyEnabled() const;
    void setLocalCapabilitiesDirectoryPersistencyEnabled(bool enabled);

    bool isGlobalCapabilitiesDirectoryCompressedMessagesEnabled() const;
    void setGlobalCapabilitiesDirectoryCompressedMessagesEnabled(bool enable);

    int getPurgeExpiredDiscoveryEntriesIntervalMs() const;
    void setPurgeExpiredDiscoveryEntriesIntervalMs(int purgeExpiredEntriesIntervalMs);

    std::chrono::milliseconds getCapabilitiesFreshnessUpdateIntervalMs() const;
    void setCapabilitiesFreshnessUpdateIntervalMs(
            std::chrono::milliseconds capabilitiesFreshnessUpdateIntervalMs);

    void setAclEntriesDirectory(const std::string& directoryPath);
    std::string getAclEntriesDirectory() const;

    void printSettings() const;

private:
    void operator=(const ClusterControllerSettings& other) = delete;

    Settings& _settings;
    ADD_LOGGER(ClusterControllerSettings)
    void checkSettings();
};

} // namespace joynr
#endif // LIBJOYNRSETTINGS_H

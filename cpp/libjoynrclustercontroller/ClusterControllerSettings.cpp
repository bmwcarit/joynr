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
#include "joynr/ClusterControllerSettings.h"

#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Settings.h"

namespace joynr
{

ClusterControllerSettings::ClusterControllerSettings(Settings& settings) : _settings(settings)
{
    settings.fillEmptySettingsWithDefaults(DEFAULT_CLUSTERCONTROLLER_SETTINGS_FILENAME());
    checkSettings();
    printSettings();
}

void ClusterControllerSettings::checkSettings()
{
    if (!_settings.contains(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME())) {
        setMulticastReceiverDirectoryPersistenceFilename(
                DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED())) {
        setMulticastReceiverDirectoryPersistencyEnabled(
                DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED());
    }

    if (!_settings.contains(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME())) {
        setLocalCapabilitiesDirectoryPersistenceFilename(
                DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED())) {
        setLocalCapabilitiesDirectoryPersistencyEnabled(
                DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED());
    }

    if (!_settings.contains(SETTING_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED())) {
        setGlobalCapabilitiesDirectoryCompressedMessagesEnabled(
                DEFAULT_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED());
    }

    if (!_settings.contains(SETTING_MQTT_CLIENT_ID_PREFIX())) {
        setMqttClientIdPrefix(DEFAULT_MQTT_CLIENT_ID_PREFIX());
    }

    if (!_settings.contains(SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME())) {
        setLocalDomainAccessStorePersistenceFilename(
                DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_MESSAGE_QUEUE_LIMIT())) {
        setMessageQueueLimit(DEFAULT_MESSAGE_QUEUE_LIMIT());
    }

    if (!_settings.contains(SETTING_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT())) {
        setPerParticipantIdMessageQueueLimit(DEFAULT_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT());
    }

    if (!_settings.contains(SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT())) {
        setTransportNotAvailableQueueLimit(DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT());
    }

    if (!_settings.contains(SETTING_MESSAGE_QUEUE_LIMIT_BYTES())) {
        setMessageQueueLimitBytes(DEFAULT_MESSAGE_QUEUE_LIMIT_BYTES());
    }

    if (!_settings.contains(SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES())) {
        setTransportNotAvailableQueueLimitBytes(
                DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES());
    }

    if (!_settings.contains(SETTING_MQTT_MULTICAST_TOPIC_PREFIX())) {
        setMqttMulticastTopicPrefix(DEFAULT_MQTT_MULTICAST_TOPIC_PREFIX());
    }
    if (!_settings.contains(SETTING_MQTT_UNICAST_TOPIC_PREFIX())) {
        setMqttMulticastTopicPrefix(DEFAULT_MQTT_UNICAST_TOPIC_PREFIX());
    }

    if (!_settings.contains(SETTING_USE_ONLY_LDAS())) {
        setUseOnlyLDAS(DEFAULT_USE_ONLY_LDAS());
    }

    if (!_settings.contains(SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS())) {
        setPurgeExpiredDiscoveryEntriesIntervalMs(
                DEFAULT_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS());
    }

    if (!_settings.contains(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS())) {
        _settings.set(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                      DEFAULT_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS().count());
    }

    if (!_settings.contains(SETTING_MQTT_TLS_ENABLED())) {
        _settings.set(SETTING_MQTT_TLS_ENABLED(), DEFAULT_MQTT_TLS_ENABLED());
    }

    if (!_settings.contains(SETTING_MQTT_TLS_VERSION())) {
        _settings.set(SETTING_MQTT_TLS_VERSION(), DEFAULT_MQTT_TLS_VERSION());
    }

    if (!_settings.contains(SETTING_MQTT_TLS_CIPHERS())) {
        _settings.set(SETTING_MQTT_TLS_CIPHERS(), DEFAULT_MQTT_TLS_CIPHERS());
    }

    if (!_settings.contains(SETTING_ACCESS_CONTROL_AUDIT())) {
        _settings.set(SETTING_ACCESS_CONTROL_AUDIT(), DEFAULT_ACCESS_CONTROL_AUDIT());
    }

    if (isMqttTlsEnabled()) {
        if (!isMqttCertificateAuthorityCertificateFolderPathSet() &&
            !isMqttCertificateAuthorityPemFilenameSet()) {
            const std::string message =
                    "MQTT TLS is enabled but no CA certificate filename or folder was provided";
            JOYNR_LOG_ERROR(logger(), message);
            throw joynr::exceptions::JoynrConfigurationException(message);
        }

        if (!isMqttPrivateKeyPemFilenameSet() && !isMqttUsernameSet()) {
            const std::string message = "MQTT TLS is enabled but neither client certificate nor "
                                        "username/password are provided";
            JOYNR_LOG_ERROR(logger(), message);
            throw joynr::exceptions::JoynrConfigurationException(message);
        }

        if (isMqttCertificatePemFilenameSet() != isMqttPrivateKeyPemFilenameSet()) {
            std::string message;
            if (!isMqttCertificatePemFilenameSet()) {
                message = "MQTT private key filename is set, but client certificate not";
            } else {
                message = "MQTT client certificate filename is set, but private key not";
            }
            JOYNR_LOG_ERROR(logger(), message);
            throw joynr::exceptions::JoynrConfigurationException(message);
        }
    } else if (isMqttCertificateAuthorityCertificateFolderPathSet() ||
               isMqttCertificateAuthorityPemFilenameSet() || isMqttCertificatePemFilenameSet() ||
               isMqttPrivateKeyPemFilenameSet()) {
        JOYNR_LOG_WARN(
                logger(), "MQTT TLS is disabled but at least one MQTT TLS property was configured");
    }

    if (!_settings.contains(SETTING_ACCESS_CONTROL_ENABLE())) {
        setEnableAccessController(DEFAULT_ENABLE_ACCESS_CONTROLLER());
    } else if (enableAccessController() && !getUseOnlyLDAS()) {
        assert(_settings.contains(
                SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS()));
        assert(_settings.contains(
                SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID()));

        if (!_settings.contains(SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS())) {
            JOYNR_LOG_ERROR(logger(),
                            "Configuration error. Access controller is enabled but "
                            "no {} was defined.",
                            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS());
        }
        if (!_settings.contains(
                    SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID())) {
            JOYNR_LOG_ERROR(logger(),
                            "Configuration error. Access controller is enabled but "
                            "no {} was defined.",
                            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID());
        }
    }
}

const std::string& ClusterControllerSettings::
        SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME()
{
    static const std::string value(
            "cluster-controller/local-capabilities-directory-persistence-file");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED()
{
    static const std::string value(
            "cluster-controller/local-capabilities-directory-persistency-enabled");
    return value;
}

const std::string& ClusterControllerSettings::
        DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME()
{
    static const std::string value("LocalCapabilitiesDirectory.persist");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME()
{
    static const std::string value(
            "cluster-controller/multicast-receiver-directory-persistence-file");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED()
{
    static const std::string value(
            "cluster-controller/multicast-receiver-directory-persistency-enabled");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_WS_TLS_PORT()
{
    static const std::string value("cluster-controller/ws-tls-port");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_WS_PORT()
{
    static const std::string value("cluster-controller/ws-port");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_USE_ONLY_LDAS()
{
    static const std::string value("access-control/use-ldas-only");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_ACCESS_CONTROL_ENABLE()
{
    static const std::string value("access-control/enable");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_ACCESS_CONTROL_AUDIT()
{
    static const std::string value("access-control/audit");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS()
{
    static const std::string value("access-control/global-domain-access-controller-address");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID()
{
    static const std::string value("access-control/global-domain-access-controller-participantid");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_CLIENT_ID_PREFIX()
{
    static const std::string value("cluster-controller/mqtt-client-id-prefix");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_MULTICAST_TOPIC_PREFIX()
{
    static const std::string value("cluster-controller/mqtt-multicast-topic-prefix");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_UNICAST_TOPIC_PREFIX()
{
    static const std::string value("cluster-controller/mqtt-unicast-topic-prefix");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_TLS_ENABLED()
{
    static const std::string value("cluster-controller/mqtt-tls-enabled");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_TLS_VERSION()
{
    static const std::string value("cluster-controller/mqtt-tls-version");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_TLS_CIPHERS()
{
    static const std::string value("cluster-controller/mqtt-tls-ciphers");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME()
{
    static const std::string value("cluster-controller/mqtt-certificate-authority-pem-filename");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH()
{
    static const std::string value(
            "cluster-controller/mqtt-certificate-authority-certificate-folder-path");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_CERTIFICATE_PEM_FILENAME()
{
    static const std::string value("cluster-controller/mqtt-certificate-pem-filename");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME()
{
    static const std::string value("cluster-controller/mqtt-private-key-pem-filename");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_USERNAME()
{
    static const std::string value("cluster-controller/mqtt-username");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MQTT_PASSWORD()
{
    static const std::string value("cluster-controller/mqtt-password");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS()
{
    static const std::string value(
            "cluster-controller/purge-expired-discovery-entries-interval-ms");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS()
{
    static const std::string value("cluster-controller/capabilities-freshness-update-interval-ms");
    return value;
}

int ClusterControllerSettings::DEFAULT_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS()
{
    return 60 * 60 * 1000; // 1 hour
}

std::chrono::milliseconds ClusterControllerSettings::
        DEFAULT_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS()
{
    static const std::chrono::milliseconds value(1UL * 60UL * 60UL * 1000UL); // 1 hour
    return value;
}

const std::string& ClusterControllerSettings::DEFAULT_MQTT_CLIENT_ID_PREFIX()
{
    static const std::string value("joynr");
    return value;
}

bool ClusterControllerSettings::DEFAULT_MQTT_TLS_ENABLED()
{
    return false;
}

const std::string& ClusterControllerSettings::DEFAULT_MQTT_TLS_VERSION()
{
    static const std::string value("tlsv1.2");
    return value;
}

const std::string& ClusterControllerSettings::DEFAULT_MQTT_TLS_CIPHERS()
{
    static const std::string value("");
    return value;
}

bool ClusterControllerSettings::DEFAULT_ACCESS_CONTROL_AUDIT()
{
    return false;
}

std::uint64_t ClusterControllerSettings::DEFAULT_MESSAGE_QUEUE_LIMIT()
{
    return 0;
}

std::uint64_t ClusterControllerSettings::DEFAULT_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT()
{
    return 0;
}

std::uint64_t ClusterControllerSettings::DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT()
{
    return 0;
}

std::uint64_t ClusterControllerSettings::DEFAULT_MESSAGE_QUEUE_LIMIT_BYTES()
{
    return 0;
}

std::uint64_t ClusterControllerSettings::DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES()
{
    return 0;
}

const std::string& ClusterControllerSettings::DEFAULT_MQTT_MULTICAST_TOPIC_PREFIX()
{
    static const std::string value("");
    return value;
}

const std::string& ClusterControllerSettings::DEFAULT_MQTT_UNICAST_TOPIC_PREFIX()
{
    static const std::string value("");
    return value;
}

const std::string& ClusterControllerSettings::
        DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME()
{
    static const std::string value("MulticastReceiverDirectory.persist");
    return value;
}

bool ClusterControllerSettings::DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED()
{
    return false;
}

bool ClusterControllerSettings::DEFAULT_ENABLE_ACCESS_CONTROLLER()
{
    return false;
}

bool ClusterControllerSettings::DEFAULT_USE_ONLY_LDAS()
{
    return true;
}

const std::string& ClusterControllerSettings::DEFAULT_CLUSTERCONTROLLER_SETTINGS_FILENAME()
{
    static const std::string value("default-clustercontroller.settings");
    return value;
}

std::string ClusterControllerSettings::getMulticastReceiverDirectoryPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME());
}

void ClusterControllerSettings::setMulticastReceiverDirectoryPersistenceFilename(
        const std::string& filename)
{
    _settings.set(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME(), filename);
}

bool ClusterControllerSettings::isMulticastReceiverDirectoryPersistencyEnabled() const
{
    return _settings.get<bool>(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED());
}

void ClusterControllerSettings::setMulticastReceiverDirectoryPersistencyEnabled(bool enabled)
{
    _settings.set(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED(), enabled);
}

bool ClusterControllerSettings::isWsTLSPortSet() const
{
    return _settings.contains(SETTING_WS_TLS_PORT());
}

std::uint16_t ClusterControllerSettings::getWsTLSPort() const
{
    return _settings.get<std::uint16_t>(SETTING_WS_TLS_PORT());
}

void ClusterControllerSettings::setWsTLSPort(std::uint16_t port)
{
    _settings.set(SETTING_WS_TLS_PORT(), port);
}

bool ClusterControllerSettings::isWsPortSet() const
{
    return _settings.contains(SETTING_WS_PORT());
}

std::uint16_t ClusterControllerSettings::getWsPort() const
{
    return _settings.get<std::uint16_t>(SETTING_WS_PORT());
}

void ClusterControllerSettings::setWsPort(std::uint16_t port)
{
    _settings.set(SETTING_WS_PORT(), port);
}

bool ClusterControllerSettings::isMqttClientIdPrefixSet() const
{
    return _settings.contains(SETTING_MQTT_CLIENT_ID_PREFIX());
}

int ClusterControllerSettings::getPurgeExpiredDiscoveryEntriesIntervalMs() const
{
    return _settings.get<int>(SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS());
}

void ClusterControllerSettings::setPurgeExpiredDiscoveryEntriesIntervalMs(
        int purgeExpiredEntriesIntervalMs)
{
    _settings.set(
            SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS(), purgeExpiredEntriesIntervalMs);
}

std::string ClusterControllerSettings::getMqttClientIdPrefix() const
{
    return _settings.get<std::string>(SETTING_MQTT_CLIENT_ID_PREFIX());
}

void ClusterControllerSettings::setMqttClientIdPrefix(const std::string& mqttClientId)
{
    _settings.set(SETTING_MQTT_CLIENT_ID_PREFIX(), mqttClientId);
}

std::string ClusterControllerSettings::getMqttMulticastTopicPrefix() const
{
    return _settings.get<std::string>(SETTING_MQTT_MULTICAST_TOPIC_PREFIX());
}

void ClusterControllerSettings::setMqttMulticastTopicPrefix(
        const std::string& mqttMulticastTopicPrefix)
{
    _settings.set(SETTING_MQTT_MULTICAST_TOPIC_PREFIX(), mqttMulticastTopicPrefix);
}

std::string ClusterControllerSettings::getMqttUnicastTopicPrefix() const
{
    return _settings.get<std::string>(SETTING_MQTT_UNICAST_TOPIC_PREFIX());
}

void ClusterControllerSettings::setMqttUnicastTopicPrefix(const std::string& mqttUnicastTopicPrefix)
{
    _settings.set(SETTING_MQTT_UNICAST_TOPIC_PREFIX(), mqttUnicastTopicPrefix);
}

bool ClusterControllerSettings::isMqttCertificateAuthorityPemFilenameSet() const
{
    return _settings.contains(SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME());
}

std::string ClusterControllerSettings::getMqttCertificateAuthorityPemFilename() const
{
    return _settings.get<std::string>(SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME());
}

bool ClusterControllerSettings::isMqttCertificateAuthorityCertificateFolderPathSet() const
{
    return _settings.contains(SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH());
}

std::string ClusterControllerSettings::getMqttCertificateAuthorityCertificateFolderPath() const
{
    return _settings.get<std::string>(SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH());
}

bool ClusterControllerSettings::isMqttCertificatePemFilenameSet() const
{
    return _settings.contains(SETTING_MQTT_CERTIFICATE_PEM_FILENAME());
}

std::string ClusterControllerSettings::getMqttCertificatePemFilename() const
{
    return _settings.get<std::string>(SETTING_MQTT_CERTIFICATE_PEM_FILENAME());
}

bool ClusterControllerSettings::isMqttPrivateKeyPemFilenameSet() const
{
    return _settings.contains(SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME());
}

std::string ClusterControllerSettings::getMqttPrivateKeyPemFilename() const
{
    return _settings.get<std::string>(SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME());
}

bool ClusterControllerSettings::isMqttUsernameSet() const
{
    return _settings.contains(SETTING_MQTT_USERNAME());
}

std::string ClusterControllerSettings::getMqttUsername() const
{
    return _settings.get<std::string>(SETTING_MQTT_USERNAME());
}

bool ClusterControllerSettings::isMqttPasswordSet() const
{
    return _settings.contains(SETTING_MQTT_PASSWORD());
}

std::string ClusterControllerSettings::getMqttPassword() const
{
    return _settings.get<std::string>(SETTING_MQTT_PASSWORD());
}

void ClusterControllerSettings::setMqttTlsEnabled(bool enabled)
{
    _settings.set<bool>(SETTING_MQTT_TLS_ENABLED(), enabled);
}

bool ClusterControllerSettings::isMqttTlsEnabled() const
{
    return _settings.get<bool>(SETTING_MQTT_TLS_ENABLED());
}

void ClusterControllerSettings::setMqttTlsVersion(const std::string& tlsVersion)
{
    _settings.set<std::string>(SETTING_MQTT_TLS_VERSION(), tlsVersion);
}

std::string ClusterControllerSettings::getMqttTlsVersion() const
{
    return _settings.get<std::string>(SETTING_MQTT_TLS_VERSION());
}

void ClusterControllerSettings::setMqttTlsCiphers(const std::string& tlsCiphers)
{
    _settings.set<std::string>(SETTING_MQTT_TLS_CIPHERS(), tlsCiphers);
}

std::string ClusterControllerSettings::getMqttTlsCiphers() const
{
    return _settings.get<std::string>(SETTING_MQTT_TLS_CIPHERS());
}

bool ClusterControllerSettings::isGlobalCapabilitiesDirectoryCompressedMessagesEnabled() const
{
    return _settings.get<bool>(SETTING_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED());
}

void ClusterControllerSettings::setGlobalCapabilitiesDirectoryCompressedMessagesEnabled(
        bool enabled)
{
    return _settings.set<bool>(
            SETTING_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED(), enabled);
}

const std::string& ClusterControllerSettings::
        DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME()
{
    static const std::string value("LocalDomainAccessStore.persist");
    return value;
}

bool ClusterControllerSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED()
{
    return false;
}

bool ClusterControllerSettings::DEFAULT_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED()
{
    return false;
}

const std::string& ClusterControllerSettings::SETTING_MESSAGE_QUEUE_LIMIT()
{
    static const std::string value("cluster-controller/message-queue-limit");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT()
{
    static const std::string value("cluster-controller/per-participantid-message-queue-limit");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT()
{
    static const std::string value("cluster-controller/transport-not-available-queue-limit");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_MESSAGE_QUEUE_LIMIT_BYTES()
{
    static const std::string value("cluster-controller/message-queue-limit-bytes");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES()
{
    static const std::string value("cluster-controller/transport-not-available-queue-limit-bytes");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME()
{
    static const std::string value("cluster-controller/local-domain-access-store-persistence-file");
    return value;
}

const std::string& ClusterControllerSettings::SETTING_ACL_ENTRIES_DIRECTORY()
{
    static const std::string value("cluster-controller/acl-entries-directory");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED()
{
    static const std::string value(
            "cluster-controller/global-capabilities-directory-compressed-messages-enabled");
    return value;
}

std::string ClusterControllerSettings::getLocalDomainAccessStorePersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
}

void ClusterControllerSettings::setLocalDomainAccessStorePersistenceFilename(
        const std::string& filename)
{
    _settings.set(SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME(), filename);
}

std::uint64_t ClusterControllerSettings::getMessageQueueLimit() const
{
    return _settings.get<std::uint64_t>(SETTING_MESSAGE_QUEUE_LIMIT());
}

void ClusterControllerSettings::setMessageQueueLimit(std::uint64_t limit)
{
    _settings.set(SETTING_MESSAGE_QUEUE_LIMIT(), limit);
}

std::uint64_t ClusterControllerSettings::getPerParticipantIdMessageQueueLimit() const
{
    return _settings.get<std::uint64_t>(SETTING_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT());
}

void ClusterControllerSettings::setPerParticipantIdMessageQueueLimit(std::uint64_t limit)
{
    _settings.set(SETTING_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT(), limit);
}

std::uint64_t ClusterControllerSettings::getTransportNotAvailableQueueLimit() const
{
    return _settings.get<std::uint64_t>(SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT());
}

void ClusterControllerSettings::setTransportNotAvailableQueueLimit(std::uint64_t limit)
{
    _settings.set(SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT(), limit);
}

std::uint64_t ClusterControllerSettings::getMessageQueueLimitBytes() const
{
    return _settings.get<std::uint64_t>(SETTING_MESSAGE_QUEUE_LIMIT_BYTES());
}

void ClusterControllerSettings::setMessageQueueLimitBytes(std::uint64_t limitBytes)
{
    _settings.set(SETTING_MESSAGE_QUEUE_LIMIT_BYTES(), limitBytes);
}

std::uint64_t ClusterControllerSettings::getTransportNotAvailableQueueLimitBytes() const
{
    return _settings.get<std::uint64_t>(SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES());
}

void ClusterControllerSettings::setTransportNotAvailableQueueLimitBytes(std::uint64_t limitBytes)
{
    _settings.set(SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES(), limitBytes);
}

void ClusterControllerSettings::setAclEntriesDirectory(const std::string& directoryPath)
{
    _settings.set(SETTING_ACL_ENTRIES_DIRECTORY(), directoryPath);
}

std::string ClusterControllerSettings::getAclEntriesDirectory() const
{
    return _settings.get<std::string>(SETTING_ACL_ENTRIES_DIRECTORY());
}

bool ClusterControllerSettings::enableAccessController() const
{
    return _settings.get<bool>(SETTING_ACCESS_CONTROL_ENABLE());
}

void ClusterControllerSettings::setEnableAccessController(bool enable)
{
    _settings.set(SETTING_ACCESS_CONTROL_ENABLE(), enable);
}

bool ClusterControllerSettings::aclAudit() const
{
    return _settings.get<bool>(SETTING_ACCESS_CONTROL_AUDIT());
}

void ClusterControllerSettings::setAclAudit(bool audit)
{
    _settings.set(SETTING_ACCESS_CONTROL_AUDIT(), audit);
}

std::string ClusterControllerSettings::getGlobalDomainAccessControlAddress() const
{
    return _settings.get<std::string>(
            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS());
}

std::string ClusterControllerSettings::getGlobalDomainAccessControlParticipantId() const
{
    return _settings.get<std::string>(
            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID());
}

bool ClusterControllerSettings::getUseOnlyLDAS() const
{
    return _settings.get<bool>(SETTING_USE_ONLY_LDAS());
}

void ClusterControllerSettings::setUseOnlyLDAS(bool useOnlyLDAS)
{
    _settings.set(SETTING_USE_ONLY_LDAS(), useOnlyLDAS);
}

std::string ClusterControllerSettings::getLocalCapabilitiesDirectoryPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME());
}

void ClusterControllerSettings::setLocalCapabilitiesDirectoryPersistenceFilename(
        const std::string& filename)
{
    _settings.set(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME(), filename);
}

std::chrono::milliseconds ClusterControllerSettings::getCapabilitiesFreshnessUpdateIntervalMs()
        const
{
    return std::chrono::milliseconds(
            _settings.get<std::uint64_t>(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS()));
}

bool ClusterControllerSettings::isLocalCapabilitiesDirectoryPersistencyEnabled() const
{
    return _settings.get<bool>(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED());
}

void ClusterControllerSettings::setLocalCapabilitiesDirectoryPersistencyEnabled(bool enabled)
{
    _settings.set(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(), enabled);
}

void ClusterControllerSettings::setCapabilitiesFreshnessUpdateIntervalMs(
        std::chrono::milliseconds capabilitiesFreshnessUpdateIntervalMs)
{
    return _settings.set(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                         capabilitiesFreshnessUpdateIntervalMs.count());
}

void ClusterControllerSettings::printSettings() const
{
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED(),
                   isMulticastReceiverDirectoryPersistencyEnabled());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME(),
                   getMulticastReceiverDirectoryPersistenceFilename());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(),
                   isLocalCapabilitiesDirectoryPersistencyEnabled());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME(),
                   getLocalCapabilitiesDirectoryPersistenceFilename());

    JOYNR_LOG_INFO(
            logger(), "SETTING: {} = {}", SETTING_MESSAGE_QUEUE_LIMIT(), getMessageQueueLimit());
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MESSAGE_QUEUE_LIMIT_BYTES(),
                   getMessageQueueLimitBytes());
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_PER_PARTICIPANTID_MESSAGE_QUEUE_LIMIT(),
                   getPerParticipantIdMessageQueueLimit());
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT(),
                   getTransportNotAvailableQueueLimit());
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES(),
                   getTransportNotAvailableQueueLimitBytes());

    JOYNR_LOG_INFO(
            logger(), "SETTING: {} = {}", SETTING_MQTT_CLIENT_ID_PREFIX(), getMqttClientIdPrefix());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MQTT_MULTICAST_TOPIC_PREFIX(),
                   getMqttMulticastTopicPrefix());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MQTT_UNICAST_TOPIC_PREFIX(),
                   getMqttUnicastTopicPrefix());

    if (isWsTLSPortSet()) {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_WS_TLS_PORT(), getWsTLSPort());
    } else {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = NOT SET", SETTING_WS_TLS_PORT());
    }

    if (isWsPortSet()) {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_WS_PORT(), getWsPort());
    } else {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = NOT SET", SETTING_WS_PORT());
    }

    JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_MQTT_TLS_ENABLED(), isMqttTlsEnabled());

    if (isMqttCertificateAuthorityPemFilenameSet()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME(),
                       getMqttCertificateAuthorityPemFilename());
    } else {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = NOT SET",
                       SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME());
    }

    if (isMqttCertificateAuthorityCertificateFolderPathSet()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH(),
                       getMqttCertificateAuthorityCertificateFolderPath());
    } else {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = NOT SET",
                       SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH());
    }

    if (isMqttCertificatePemFilenameSet()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_MQTT_CERTIFICATE_PEM_FILENAME(),
                       getMqttCertificatePemFilename());
    } else {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = NOT SET", SETTING_MQTT_CERTIFICATE_PEM_FILENAME());
    }

    if (isMqttPrivateKeyPemFilenameSet()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME(),
                       getMqttPrivateKeyPemFilename());
    } else {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = NOT SET", SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME());
    }

    if (isMqttUsernameSet()) {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_MQTT_USERNAME(), getMqttUsername());
    } else {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = NOT SET", SETTING_MQTT_USERNAME());
    }

    if (isMqttPasswordSet()) {
        JOYNR_LOG_INFO(logger(), "SETTING: {} IS SET", SETTING_MQTT_PASSWORD());
    } else {
        JOYNR_LOG_INFO(logger(), "SETTING: {} = NOT SET", SETTING_MQTT_PASSWORD());
    }

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME(),
                   getLocalDomainAccessStorePersistenceFilename());

    JOYNR_LOG_INFO(logger(), "SETTING: {} = {}", SETTING_USE_ONLY_LDAS(), getUseOnlyLDAS());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {})",
                   SETTING_ACCESS_CONTROL_ENABLE(),
                   _settings.get<std::string>(SETTING_ACCESS_CONTROL_ENABLE()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {})",
                   SETTING_ACCESS_CONTROL_AUDIT(),
                   _settings.get<std::string>(SETTING_ACCESS_CONTROL_AUDIT()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {})",
                   SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                   getCapabilitiesFreshnessUpdateIntervalMs().count());
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {})",
                   SETTING_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED(),
                   isGlobalCapabilitiesDirectoryCompressedMessagesEnabled());

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {})",
                   SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS(),
                   getPurgeExpiredDiscoveryEntriesIntervalMs());

    if (_settings.get<bool>(SETTING_ACCESS_CONTROL_ENABLE())) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {})",
                       SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS(),
                       _settings.get<std::string>(
                               SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS()));
        JOYNR_LOG_INFO(
                logger(),
                "SETTING: {} = {})",
                SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID(),
                _settings.get<std::string>(
                        SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID()));
    }
}

} // namespace joynr

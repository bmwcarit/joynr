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
#include "joynr/Settings.h"

namespace joynr
{

INIT_LOGGER(ClusterControllerSettings);

ClusterControllerSettings::ClusterControllerSettings(Settings& settings) : settings(settings)
{
    settings.fillEmptySettingsWithDefaults(DEFAULT_CLUSTERCONTROLLER_SETTINGS_FILENAME());
    checkSettings();
    printSettings();
}

void ClusterControllerSettings::checkSettings()
{
    if (!settings.contains(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME())) {
        setMulticastReceiverDirectoryPersistenceFilename(
                DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME());
    }

    if (!settings.contains(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME())) {
        setLocalCapabilitiesDirectoryPersistenceFilename(
                DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME());
    }

    if (!settings.contains(SETTING_MQTT_CLIENT_ID_PREFIX())) {
        setMqttClientIdPrefix(DEFAULT_MQTT_CLIENT_ID_PREFIX());
    }

    if (!settings.contains(SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME())) {
        setLocalDomainAccessStorePersistenceFilename(
                DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
    }
    if (!settings.contains(SETTING_MQTT_MULTICAST_TOPIC_PREFIX())) {
        setMqttMulticastTopicPrefix(DEFAULT_MQTT_MULTICAST_TOPIC_PREFIX());
    }
    if (!settings.contains(SETTING_MQTT_UNICAST_TOPIC_PREFIX())) {
        setMqttMulticastTopicPrefix(DEFAULT_MQTT_UNICAST_TOPIC_PREFIX());
    }

    if (!settings.contains(SETTING_USE_ONLY_LDAS())) {
        setUseOnlyLDAS(DEFAULT_USE_ONLY_LDAS());
    }

    if (!settings.contains(SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS())) {
        setPurgeExpiredDiscoveryEntriesIntervalMs(
                DEFAULT_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS());
    }

    if (!settings.contains(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS())) {
        settings.set(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                     DEFAULT_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS().count());
    }

    if (!settings.contains(SETTING_MQTT_TLS_ENABLED())) {
        settings.set(SETTING_MQTT_TLS_ENABLED(), DEFAULT_MQTT_TLS_ENABLED());
    }

    if (!settings.contains(SETTING_ACCESS_CONTROL_ENABLE())) {
        setEnableAccessController(DEFAULT_ENABLE_ACCESS_CONTROLLER());
    } else if (enableAccessController() && !getUseOnlyLDAS()) {
        assert(settings.contains(SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS()));
        assert(settings.contains(
                SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID()));

        if (!settings.contains(SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS())) {
            JOYNR_LOG_ERROR(logger,
                            "Configuration error. Access controller is enabled but "
                            "no {} was defined.",
                            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS());
        }
        if (!settings.contains(
                    SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID())) {
            JOYNR_LOG_ERROR(logger,
                            "Configuration error. Access controller is enabled but "
                            "no {} was defined.",
                            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID());
        }
    }
}

const std::string& ClusterControllerSettings::
        SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME()
{
    static const std::string value("lib-joynr/local-capabilities-directory-persistence-file");
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
    return settings.get<std::string>(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME());
}

void ClusterControllerSettings::setMulticastReceiverDirectoryPersistenceFilename(
        const std::string& filename)
{
    settings.set(SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME(), filename);
}

bool ClusterControllerSettings::isWsTLSPortSet() const
{
    return settings.contains(SETTING_WS_TLS_PORT());
}

std::uint16_t ClusterControllerSettings::getWsTLSPort() const
{
    return settings.get<std::uint16_t>(SETTING_WS_TLS_PORT());
}

void ClusterControllerSettings::setWsTLSPort(std::uint16_t port)
{
    settings.set(SETTING_WS_TLS_PORT(), port);
}

bool ClusterControllerSettings::isWsPortSet() const
{
    return settings.contains(SETTING_WS_PORT());
}

std::uint16_t ClusterControllerSettings::getWsPort() const
{
    return settings.get<std::uint16_t>(SETTING_WS_PORT());
}

void ClusterControllerSettings::setWsPort(std::uint16_t port)
{
    settings.set(SETTING_WS_PORT(), port);
}

bool ClusterControllerSettings::isMqttClientIdPrefixSet() const
{
    return settings.contains(SETTING_MQTT_CLIENT_ID_PREFIX());
}

int ClusterControllerSettings::getPurgeExpiredDiscoveryEntriesIntervalMs() const
{
    return settings.get<int>(SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS());
}

void ClusterControllerSettings::setPurgeExpiredDiscoveryEntriesIntervalMs(
        int purgeExpiredEntriesIntervalMs)
{
    settings.set(
            SETTING_PURGE_EXPIRED_DISCOVERY_ENTRIES_INTERVAL_MS(), purgeExpiredEntriesIntervalMs);
}

std::string ClusterControllerSettings::getMqttClientIdPrefix() const
{
    return settings.get<std::string>(SETTING_MQTT_CLIENT_ID_PREFIX());
}

void ClusterControllerSettings::setMqttClientIdPrefix(const std::string& mqttClientId)
{
    settings.set(SETTING_MQTT_CLIENT_ID_PREFIX(), mqttClientId);
}

std::string ClusterControllerSettings::getMqttMulticastTopicPrefix() const
{
    return settings.get<std::string>(SETTING_MQTT_MULTICAST_TOPIC_PREFIX());
}

void ClusterControllerSettings::setMqttMulticastTopicPrefix(
        const std::string& mqttMulticastTopicPrefix)
{
    settings.set(SETTING_MQTT_MULTICAST_TOPIC_PREFIX(), mqttMulticastTopicPrefix);
}

std::string ClusterControllerSettings::getMqttUnicastTopicPrefix() const
{
    return settings.get<std::string>(SETTING_MQTT_UNICAST_TOPIC_PREFIX());
}

void ClusterControllerSettings::setMqttUnicastTopicPrefix(const std::string& mqttUnicastTopicPrefix)
{
    settings.set(SETTING_MQTT_UNICAST_TOPIC_PREFIX(), mqttUnicastTopicPrefix);
}

bool ClusterControllerSettings::isMqttCertificateAuthorityPemFilenameSet() const
{
    return settings.contains(SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME());
}

std::string ClusterControllerSettings::getMqttCertificateAuthorityPemFilename() const
{
    return settings.get<std::string>(SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME());
}

bool ClusterControllerSettings::isMqttCertificateAuthorityCertificateFolderPathSet() const
{
    return settings.contains(SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH());
}

std::string ClusterControllerSettings::getMqttCertificateAuthorityCertificateFolderPath() const
{
    return settings.get<std::string>(SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH());
}

bool ClusterControllerSettings::isMqttCertificatePemFilenameSet() const
{
    return settings.contains(SETTING_MQTT_CERTIFICATE_PEM_FILENAME());
}

std::string ClusterControllerSettings::getMqttCertificatePemFilename() const
{
    return settings.get<std::string>(SETTING_MQTT_CERTIFICATE_PEM_FILENAME());
}

bool ClusterControllerSettings::isMqttPrivateKeyPemFilenameSet() const
{
    return settings.contains(SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME());
}

std::string ClusterControllerSettings::getMqttPrivateKeyPemFilename() const
{
    return settings.get<std::string>(SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME());
}

void ClusterControllerSettings::setMqttTlsEnabled(bool enabled)
{
    settings.set<bool>(SETTING_MQTT_TLS_ENABLED(), enabled);
}

bool ClusterControllerSettings::isMqttTlsEnabled() const
{
    return settings.get<bool>(SETTING_MQTT_TLS_ENABLED());
}

const std::string& ClusterControllerSettings::
        DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME()
{
    static const std::string value("LocalDomainAccessStore.persist");
    return value;
}

const std::string& ClusterControllerSettings::
        SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME()
{
    static const std::string value("cluster-controller/local-domain-access-store-persistence-file");
    return value;
}
std::string ClusterControllerSettings::getLocalDomainAccessStorePersistenceFilename() const
{
    return settings.get<std::string>(SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
}

void ClusterControllerSettings::setLocalDomainAccessStorePersistenceFilename(
        const std::string& filename)
{
    settings.set(SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME(), filename);
}

bool ClusterControllerSettings::enableAccessController() const
{
    return settings.get<bool>(SETTING_ACCESS_CONTROL_ENABLE());
}

void ClusterControllerSettings::setEnableAccessController(bool enable)
{
    settings.set(SETTING_ACCESS_CONTROL_ENABLE(), enable);
}

std::string ClusterControllerSettings::getGlobalDomainAccessControlAddress() const
{
    return settings.get<std::string>(
            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS());
}

std::string ClusterControllerSettings::getGlobalDomainAccessControlParticipantId() const
{
    return settings.get<std::string>(
            SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID());
}

bool ClusterControllerSettings::getUseOnlyLDAS() const
{
    return settings.get<bool>(SETTING_USE_ONLY_LDAS());
}

void ClusterControllerSettings::setUseOnlyLDAS(bool useOnlyLDAS)
{
    settings.set(SETTING_USE_ONLY_LDAS(), useOnlyLDAS);
}

std::string ClusterControllerSettings::getLocalCapabilitiesDirectoryPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME());
}

void ClusterControllerSettings::setLocalCapabilitiesDirectoryPersistenceFilename(
        const std::string& filename)
{
    settings.set(SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME(), filename);
}

std::chrono::milliseconds ClusterControllerSettings::getCapabilitiesFreshnessUpdateIntervalMs()
        const
{
    return std::chrono::milliseconds(
            settings.get<std::uint64_t>(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS()));
}

void ClusterControllerSettings::setCapabilitiesFreshnessUpdateIntervalMs(
        std::chrono::milliseconds capabilitiesFreshnessUpdateIntervalMs)
{
    return settings.set(SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                        capabilitiesFreshnessUpdateIntervalMs.count());
}

void ClusterControllerSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME(),
                    getMulticastReceiverDirectoryPersistenceFilename());

    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME(),
                    getLocalCapabilitiesDirectoryPersistenceFilename());

    JOYNR_LOG_DEBUG(
            logger, "SETTING: {}  = {}", SETTING_MQTT_CLIENT_ID_PREFIX(), getMqttClientIdPrefix());

    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_MQTT_MULTICAST_TOPIC_PREFIX(),
                    getMqttMulticastTopicPrefix());

    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_MQTT_UNICAST_TOPIC_PREFIX(),
                    getMqttUnicastTopicPrefix());

    if (isWsTLSPortSet()) {
        JOYNR_LOG_DEBUG(logger, "SETTING: {}  = {}", SETTING_WS_TLS_PORT(), getWsTLSPort());
    } else {
        JOYNR_LOG_DEBUG(logger, "SETTING: {}  = NOT SET", SETTING_WS_TLS_PORT());
    }

    if (isWsPortSet()) {
        JOYNR_LOG_DEBUG(logger, "SETTING: {}  = {}", SETTING_WS_PORT(), getWsPort());
    } else {
        JOYNR_LOG_DEBUG(logger, "SETTING: {}  = NOT SET", SETTING_WS_PORT());
    }

    JOYNR_LOG_DEBUG(logger, "SETTING: {}  = {}", SETTING_MQTT_TLS_ENABLED(), isMqttTlsEnabled());

    if (isMqttCertificateAuthorityPemFilenameSet()) {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = {}",
                        SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME(),
                        getMqttCertificateAuthorityPemFilename());
    } else {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = NOT SET",
                        SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME());
    }

    if (isMqttCertificateAuthorityCertificateFolderPathSet()) {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = {}",
                        SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH(),
                        getMqttCertificateAuthorityCertificateFolderPath());
    } else {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = NOT SET",
                        SETTING_MQTT_CERTIFICATE_AUTHORITY_CERTIFICATE_FOLDER_PATH());
    }

    if (isMqttCertificatePemFilenameSet()) {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = {}",
                        SETTING_MQTT_CERTIFICATE_PEM_FILENAME(),
                        getMqttCertificatePemFilename());
    } else {
        JOYNR_LOG_DEBUG(logger, "SETTING: {}  = NOT SET", SETTING_MQTT_CERTIFICATE_PEM_FILENAME());
    }

    if (isMqttPrivateKeyPemFilenameSet()) {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = {}",
                        SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME(),
                        getMqttPrivateKeyPemFilename());
    } else {
        JOYNR_LOG_DEBUG(logger, "SETTING: {}  = NOT SET", SETTING_MQTT_PRIVATE_KEY_PEM_FILENAME());
    }

    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME(),
                    getLocalDomainAccessStorePersistenceFilename());

    JOYNR_LOG_DEBUG(logger, "SETTING: {}  = {}", SETTING_USE_ONLY_LDAS(), getUseOnlyLDAS());

    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {})",
                    SETTING_ACCESS_CONTROL_ENABLE(),
                    settings.get<std::string>(SETTING_ACCESS_CONTROL_ENABLE()));
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {} = {})",
                    SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                    getCapabilitiesFreshnessUpdateIntervalMs().count());
    if (settings.get<bool>(SETTING_ACCESS_CONTROL_ENABLE())) {
        JOYNR_LOG_DEBUG(logger,
                        "SETTING: {}  = {})",
                        SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS(),
                        settings.get<std::string>(
                                SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_ADDRESS()));
        JOYNR_LOG_DEBUG(
                logger,
                "SETTING: {}  = {})",
                SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID(),
                settings.get<std::string>(
                        SETTING_ACCESS_CONTROL_GLOBAL_DOMAIN_ACCESS_CONTROLLER_PARTICIPANTID()));
    }
}

} // namespace joynr

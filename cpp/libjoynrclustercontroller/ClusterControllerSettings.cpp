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

const std::string& ClusterControllerSettings::SETTING_MQTT_CERTIFICATE_AUTHORITY_PEM_FILENAME()
{
    static const std::string value("cluster-controller/mqtt-certificate-authority-pem-filename");
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

const std::string& ClusterControllerSettings::DEFAULT_MQTT_CLIENT_ID_PREFIX()
{
    static const std::string value("joynr");
    return value;
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

bool ClusterControllerSettings::isMqttTlsEnabled() const
{
    return isMqttCertificateAuthorityPemFilenameSet() && isMqttCertificatePemFilenameSet() &&
           isMqttPrivateKeyPemFilenameSet();
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

void ClusterControllerSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME(),
                    getMulticastReceiverDirectoryPersistenceFilename());

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
}

} // namespace joynr

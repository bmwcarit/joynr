/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "joynr/Settings.h"
#include "joynr/Logger.h"

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

void ClusterControllerSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME(),
                    getMulticastReceiverDirectoryPersistenceFilename());

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
}

} // namespace joynr

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
#include "joynr/UdsSettings.h"

#include <cassert>
#include <cstdint>

#include "joynr/Settings.h"
#include "joynr/Util.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"

namespace joynr
{

UdsSettings::UdsSettings(Settings& settings) : _settings(settings)
{
    _settings.fillEmptySettingsWithDefaults(DEFAULT_UDS_SETTINGS_FILENAME());
    checkSettings();
}

void UdsSettings::checkSettings()
{
    if (!_settings.contains(SETTING_SOCKET_PATH())) {
        setSocketPath(DEFAULT_SOCKET_PATH());
    }

    if (!_settings.contains(SETTING_RECONNECT_SLEEP_TIME_MS())) {
        setReconnectSleepTimeMs(DEFAULT_RECONNECT_SLEEP_TIME_MS());
    }

    if (!_settings.contains(SETTING_CLIENT_ID())) {
        setClientId(joynr::util::createUuid());
    }
}

const std::string& UdsSettings::SETTING_SOCKET_PATH()
{
    static const std::string value("uds/socket-path");
    return value;
}

const std::string& UdsSettings::SETTING_RECONNECT_SLEEP_TIME_MS()
{
    static const std::string value("uds/reconnect-sleep-time-ms");
    return value;
}

const std::string& UdsSettings::SETTING_CLIENT_ID()
{
    static const std::string value("uds/client-id");
    return value;
}

const std::string& UdsSettings::DEFAULT_UDS_SETTINGS_FILENAME()
{
    static const std::string value("default-uds.settings");
    return value;
}

std::string UdsSettings::getSocketPath() const
{
    return _settings.get<std::string>(UdsSettings::SETTING_SOCKET_PATH());
}

void UdsSettings::setSocketPath(const std::string& socketPath)
{
    _settings.set(UdsSettings::SETTING_SOCKET_PATH(), socketPath);
}

std::chrono::milliseconds UdsSettings::getReconnectSleepTimeMs() const
{
    return std::chrono::milliseconds(
            _settings.get<std::int64_t>(UdsSettings::SETTING_RECONNECT_SLEEP_TIME_MS()));
}

void UdsSettings::setReconnectSleepTimeMs(const std::chrono::milliseconds reconnectSleepTimeMs)
{
    _settings.set(UdsSettings::SETTING_RECONNECT_SLEEP_TIME_MS(), reconnectSleepTimeMs.count());
}

std::string UdsSettings::getClientId() const
{
    return _settings.get<std::string>(UdsSettings::SETTING_CLIENT_ID());
}

void UdsSettings::setClientId(const std::string& clientId)
{
    _settings.set(UdsSettings::SETTING_CLIENT_ID(), clientId);
}

const std::string& UdsSettings::DEFAULT_SOCKET_PATH()
{
    static const std::string value("/var/run/joynr/cluster-controller.sock");
    return value;
}

std::chrono::milliseconds UdsSettings::DEFAULT_RECONNECT_SLEEP_TIME_MS()
{
    static const std::chrono::milliseconds value(500);
    return value;
}

joynr::system::RoutingTypes::UdsAddress UdsSettings::createClusterControllerMessagingAddress() const
{
    return system::RoutingTypes::UdsAddress(getSocketPath());
}

bool UdsSettings::contains(const std::string& key) const
{
    return _settings.contains(key);
}

void UdsSettings::printSettings() const
{
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_SOCKET_PATH(),
                   _settings.get<std::string>(SETTING_SOCKET_PATH()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_RECONNECT_SLEEP_TIME_MS(),
                   _settings.get<std::string>(SETTING_RECONNECT_SLEEP_TIME_MS()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CLIENT_ID(),
                   _settings.get<std::string>(SETTING_CLIENT_ID()));
}

} // namespace joynr

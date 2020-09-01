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
#include <limits>

#include "joynr/Settings.h"
#include "joynr/Util.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

namespace joynr
{

const std::string& UdsSettings::DEFAULT_UDS_SETTINGS_FILENAME()
{
    static const std::string value("default-uds.settings");
    return value;
}

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

    if (!_settings.contains(SETTING_CONNECT_SLEEP_TIME_MS())) {
        setConnectSleepTimeMs(DEFAULT_CONNECT_SLEEP_TIME_MS());
    }

    if (!_settings.contains(SETTING_CLIENT_ID())) {
        setClientId(joynr::util::createUuid());
    }

    if (!_settings.contains(SETTING_SENDING_QUEUE_SIZE())) {
        setSendingQueueSize(DEFAULT_SENDING_QUEUE_SIZE());
    }
}

const std::string& UdsSettings::SETTING_SOCKET_PATH()
{
    static const std::string value("uds/socket-path");
    return value;
}

const std::string& UdsSettings::SETTING_CONNECT_SLEEP_TIME_MS()
{
    static const std::string value("uds/connect-sleep-time-ms");
    return value;
}

const std::string& UdsSettings::SETTING_CLIENT_ID()
{
    static const std::string value("uds/client-id");
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

std::chrono::milliseconds UdsSettings::getConnectSleepTimeMs() const
{
    return std::chrono::milliseconds(
            _settings.get<std::int64_t>(UdsSettings::SETTING_CONNECT_SLEEP_TIME_MS()));
}

void UdsSettings::setConnectSleepTimeMs(const std::chrono::milliseconds connectSleepTimeMs)
{
    _settings.set(UdsSettings::SETTING_CONNECT_SLEEP_TIME_MS(), connectSleepTimeMs.count());
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

std::chrono::milliseconds UdsSettings::DEFAULT_CONNECT_SLEEP_TIME_MS()
{
    static const std::chrono::milliseconds value(500);
    return value;
}

const std::string& UdsSettings::SETTING_SENDING_QUEUE_SIZE()
{
    static const std::string value("uds/sending-queue-size");
    return value;
}

const std::size_t& UdsSettings::DEFAULT_SENDING_QUEUE_SIZE()
{
    static const std::size_t value{1024};
    return value;
}
std::size_t UdsSettings::getSendingQueueSize() const
{
    const auto sendingQueueSizeStr =
            _settings.get<std::string>(UdsSettings::SETTING_SENDING_QUEUE_SIZE());
    try {
        const auto sendingQueueSize = std::stoul(sendingQueueSizeStr);
        return sendingQueueSize; // In case of zero, an additional message will trigger a reschedule
                                 // if one message is still processed.
    } catch (const std::logic_error& ex) {
        JOYNR_LOG_ERROR(logger(),
                        "Cannot parse ",
                        UdsSettings::SETTING_SENDING_QUEUE_SIZE(),
                        " value '",
                        sendingQueueSizeStr,
                        " '. Exception: ",
                        ex.what());
    }
    return DEFAULT_SENDING_QUEUE_SIZE();
}

void UdsSettings::setSendingQueueSize(const std::size_t& queueSize)
{
    _settings.set(UdsSettings::SETTING_SENDING_QUEUE_SIZE(), std::to_string(queueSize));
}

joynr::system::RoutingTypes::UdsAddress UdsSettings::createClusterControllerMessagingAddress() const
{
    return system::RoutingTypes::UdsAddress(getSocketPath());
}

system::RoutingTypes::UdsClientAddress UdsSettings::createClientMessagingAddress() const
{
    return system::RoutingTypes::UdsClientAddress(getClientId());
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
                   SETTING_CONNECT_SLEEP_TIME_MS(),
                   _settings.get<std::string>(SETTING_CONNECT_SLEEP_TIME_MS()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CLIENT_ID(),
                   _settings.get<std::string>(SETTING_CLIENT_ID()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_SENDING_QUEUE_SIZE(),
                   _settings.get<std::string>(SETTING_SENDING_QUEUE_SIZE()));
}

} // namespace joynr

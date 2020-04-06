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
#ifndef UDSSETTINGS_H
#define UDSSETTINGS_H

#include <chrono>
#include <string>

#include "joynr/Logger.h"

namespace joynr
{

class Settings;

namespace system
{
namespace RoutingTypes
{
class UdsAddress;
} // namespace RoutingTypes
} // namespace system

class UdsSettings
{
public:
    static const std::string& SETTING_SOCKET_PATH();
    static const std::string& SETTING_RECONNECT_SLEEP_TIME_MS();
    static const std::string& SETTING_CLIENT_ID();

    static const std::string& DEFAULT_UDS_SETTINGS_FILENAME();

    static std::chrono::milliseconds DEFAULT_RECONNECT_SLEEP_TIME_MS();
    static const std::string& DEFAULT_SOCKET_PATH();

    explicit UdsSettings(Settings& settings);
    UdsSettings(const UdsSettings&) = default;
    UdsSettings(UdsSettings&&) = default;

    ~UdsSettings() = default;

    std::string getSocketPath() const;
    void setSocketPath(const std::string& url);

    system::RoutingTypes::UdsAddress createClusterControllerMessagingAddress() const;

    std::chrono::milliseconds getReconnectSleepTimeMs() const;
    void setReconnectSleepTimeMs(const std::chrono::milliseconds reconnectSleepTimeMs);

    std::string getClientId() const;
    void setClientId(const std::string& url);

    void printSettings() const;

    bool contains(const std::string& key) const;

private:
    void operator=(const UdsSettings& other);

    Settings& _settings;
    ADD_LOGGER(UdsSettings)
    void checkSettings();
};

} // namespace joynr
#endif // UDSSETTINGS_H

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
#ifndef CLUSTERCONTROLLERSETTINGS_H
#define CLUSTERCONTROLLERSETTINGS_H

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

#include <string>

namespace joynr
{

class Settings;

class JOYNR_EXPORT ClusterControllerSettings
{
public:
    static const std::string& SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME();
    static const std::string& SETTING_WS_TLS_PORT();
    static const std::string& SETTING_WS_PORT();

    static const std::string& DEFAULT_CLUSTERCONTROLLER_SETTINGS_FILENAME();
    static const std::string& DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME();

    explicit ClusterControllerSettings(Settings& settings);
    ClusterControllerSettings(const ClusterControllerSettings&) = default;
    ClusterControllerSettings(ClusterControllerSettings&&) = default;

    ~ClusterControllerSettings() = default;

    std::string getMulticastReceiverDirectoryPersistenceFilename() const;
    void setMulticastReceiverDirectoryPersistenceFilename(const std::string& filename);

    bool isWsTLSPortSet() const;
    std::uint16_t getWsTLSPort() const;
    void setWsTLSPort(std::uint16_t port);

    bool isWsPortSet() const;
    std::uint16_t getWsPort() const;
    void setWsPort(std::uint16_t port);

    void printSettings() const;

private:
    void operator=(const ClusterControllerSettings& other) = delete;

    Settings& settings;
    ADD_LOGGER(ClusterControllerSettings);
    void checkSettings();
};

} // namespace joynr
#endif // LIBJOYNRSETTINGS_H

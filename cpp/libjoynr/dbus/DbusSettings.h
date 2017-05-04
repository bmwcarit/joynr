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
#ifndef DBUSSETTINGS_H
#define DBUSSETTINGS_H

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

#include <string>

namespace joynr
{

class Settings;

class JOYNR_EXPORT DbusSettings
{
public:
    static const std::string& SETTING_CC_MESSAGING_DOMAIN();
    static const std::string& SETTING_CC_MESSAGING_SERVICENAME();
    static const std::string& SETTING_CC_MESSAGING_PARTICIPANTID();

    static const std::string& DEFAULT_DBUS_SETTINGS_FILENAME();

    explicit DbusSettings(Settings& settings);
    DbusSettings(const DbusSettings& other);

    ~DbusSettings();

    std::string getClusterControllerMessagingDomain() const;
    void setClusterControllerMessagingDomain(const std::string& domain);
    std::string getClusterControllerMessagingServiceName() const;
    void setClusterControllerMessagingServiceName(const std::string& serviceName);
    std::string getClusterControllerMessagingParticipantId() const;
    void setClusterControllerMessagingParticipantId(const std::string& participantId);
    std::string createClusterControllerMessagingAddressString() const;

    void printSettings() const;

private:
    void operator=(const DbusSettings& other);

    Settings& settings;
    ADD_LOGGER(DbusSettings);
    void checkSettings() const;
};

} // namespace joynr
#endif // DBUSSETTINGS_H

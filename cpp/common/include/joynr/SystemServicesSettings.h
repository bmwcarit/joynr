/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef SYSTEMSERVICESSETTINGS_H
#define SYSTEMSERVICESSETTINGS_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/Settings.h"
#include "joynr/joynrlogging.h"

#include <string>

namespace joynr
{

class JOYNRCOMMON_EXPORT SystemServicesSettings
{
public:
    explicit SystemServicesSettings(Settings& settings);
    SystemServicesSettings(const SystemServicesSettings& other);

    ~SystemServicesSettings();

    static const std::string& SETTING_DOMAIN();
    static const std::string& SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN();
    static const std::string& SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID();
    static const std::string& SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN();
    static const std::string& SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID();

    static const std::string& DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME();

    std::string getDomain() const;
    void setJoynrSystemServicesDomain(const std::string& systemServicesDomain);
    std::string getCcRoutingProviderAuthenticationToken() const;
    void setCcRoutingProviderAuthenticationToken(const std::string& authenticationToken);
    std::string getCcRoutingProviderParticipantId() const;
    void setCcRoutingProviderParticipantId(const std::string& participantId);
    std::string getCcDiscoveryProviderAuthenticationToken() const;
    void setCcDiscoveryProviderAuthenticationToken(const std::string& authenticationToken);
    std::string getCcDiscoveryProviderParticipantId() const;
    void setCcDiscoveryProviderParticipantId(const std::string& participantId);

    bool contains(const std::string& key) const;

    void printSettings() const;

private:
    void operator=(const SystemServicesSettings& other);

    Settings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};

} // namespace joynr
#endif // SYSTEMSERVICESSETTINGS_H

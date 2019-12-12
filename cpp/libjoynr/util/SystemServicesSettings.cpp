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
#include "joynr/SystemServicesSettings.h"

#include <cassert>

#include "joynr/Settings.h"

namespace joynr
{

SystemServicesSettings::SystemServicesSettings(Settings& settings) : _settings(settings)
{
    _settings.fillEmptySettingsWithDefaults(DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME());
    checkSettings();
}

const std::string& SystemServicesSettings::SETTING_DOMAIN()
{
    static const std::string value("system.services/domain");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()
{
    static const std::string value("system.services/cc-routingprovider-participantid");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()
{
    static const std::string value("system.services/cc-discoveryprovider-participantid");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_MESSAGENOTIFICATIONPROVIDER_PARTICIPANTID()
{
    static const std::string value("system.services/cc-messagenotificationprovider-participantid");
    return value;
}

const std::string& SystemServicesSettings::
        SETTING_CC_ACCESSCONTROLLISTEDITORPROVIDER_PARTICIPANTID()
{
    static const std::string value(
            "system.services/cc-accesscontrollisteditorprovider-participantid");
    return value;
}

const std::string& SystemServicesSettings::DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME()
{
    static const std::string value("default-system-services.settings");
    return value;
}

std::string SystemServicesSettings::getDomain() const
{
    return _settings.get<std::string>(SETTING_DOMAIN());
}

void SystemServicesSettings::setJoynrSystemServicesDomain(const std::string& systemServicesDomain)
{
    _settings.set(SETTING_DOMAIN(), systemServicesDomain);
}

std::string SystemServicesSettings::getCcRoutingProviderParticipantId() const
{
    return _settings.get<std::string>(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID());
}

void SystemServicesSettings::setCcRoutingProviderParticipantId(const std::string& participantId)
{
    _settings.set(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID(), participantId);
}

std::string SystemServicesSettings::getCcDiscoveryProviderParticipantId() const
{
    return _settings.get<std::string>(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID());
}

void SystemServicesSettings::setCcDiscoveryProviderParticipantId(const std::string& participantId)
{
    _settings.set(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID(), participantId);
}

std::string SystemServicesSettings::getCcMessageNotificationProviderParticipantId() const
{
    return _settings.get<std::string>(SETTING_CC_MESSAGENOTIFICATIONPROVIDER_PARTICIPANTID());
}

void SystemServicesSettings::setCcMessageNotificationProviderParticipantId(
        const std::string& participantId)
{
    _settings.set(SETTING_CC_MESSAGENOTIFICATIONPROVIDER_PARTICIPANTID(), participantId);
}

std::string SystemServicesSettings::getCcAccessControlListEditorProviderParticipantId() const
{
    return _settings.get<std::string>(SETTING_CC_ACCESSCONTROLLISTEDITORPROVIDER_PARTICIPANTID());
}

void SystemServicesSettings::setCcAccessControlListEditorProviderParticipantId(
        const std::string& participantId)
{
    _settings.set(SETTING_CC_ACCESSCONTROLLISTEDITORPROVIDER_PARTICIPANTID(), participantId);
}

bool SystemServicesSettings::contains(const std::string& key) const
{
    return _settings.contains(key);
}

// Checks messaging settings and sets defaults
void SystemServicesSettings::checkSettings() const
{
    assert(_settings.contains(SETTING_DOMAIN()));
    assert(_settings.contains(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()));
    assert(_settings.contains(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()));
    assert(_settings.contains(SETTING_CC_MESSAGENOTIFICATIONPROVIDER_PARTICIPANTID()));
}

void SystemServicesSettings::printSettings() const
{
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_DOMAIN(),
                   _settings.get<std::string>(SETTING_DOMAIN()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID(),
                   _settings.get<std::string>(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()));
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID(),
                   _settings.get<std::string>(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()));
    JOYNR_LOG_INFO(
            logger(),
            "SETTING: {} = {}",
            SETTING_CC_MESSAGENOTIFICATIONPROVIDER_PARTICIPANTID(),
            _settings.get<std::string>(SETTING_CC_MESSAGENOTIFICATIONPROVIDER_PARTICIPANTID()));
}

} // namespace joynr

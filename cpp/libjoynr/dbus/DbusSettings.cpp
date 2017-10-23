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
#include "DbusSettings.h"
#include "joynr/Settings.h"

#include <cassert>

namespace joynr
{

DbusSettings::DbusSettings(Settings& settings) : settings(settings)
{
    settings.fillEmptySettingsWithDefaults(DEFAULT_DBUS_SETTINGS_FILENAME());
    checkSettings();
}

DbusSettings::DbusSettings(const DbusSettings& other) : settings(other.settings)
{
}

DbusSettings::~DbusSettings()
{
}

void DbusSettings::checkSettings() const
{
    assert(settings.contains(SETTING_CC_MESSAGING_DOMAIN()));
    assert(settings.contains(SETTING_CC_MESSAGING_SERVICENAME()));
    assert(settings.contains(SETTING_CC_MESSAGING_PARTICIPANTID()));
}

const std::string& DbusSettings::SETTING_CC_MESSAGING_DOMAIN()
{
    static const std::string value("dbus/cluster-controller-messaging-domain");
    return value;
}

const std::string& DbusSettings::SETTING_CC_MESSAGING_SERVICENAME()
{
    static const std::string value("dbus/cluster-controller-messaging-servicename");
    return value;
}

const std::string& DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID()
{
    static const std::string value("dbus/cluster-controller-messaging-participantid");
    return value;
}

const std::string& DbusSettings::DEFAULT_DBUS_SETTINGS_FILENAME()
{
    static const std::string value("default-dbus.settings");
    return value;
}

std::string DbusSettings::getClusterControllerMessagingDomain() const
{
    return settings.get<std::string>(DbusSettings::SETTING_CC_MESSAGING_DOMAIN());
}

void DbusSettings::setClusterControllerMessagingDomain(const std::string& domain)
{
    settings.set(DbusSettings::SETTING_CC_MESSAGING_DOMAIN(), domain);
}

std::string DbusSettings::getClusterControllerMessagingServiceName() const
{
    return settings.get<std::string>(DbusSettings::SETTING_CC_MESSAGING_SERVICENAME());
}

void DbusSettings::setClusterControllerMessagingServiceName(const std::string& serviceName)
{
    settings.set(DbusSettings::SETTING_CC_MESSAGING_SERVICENAME(), serviceName);
}

std::string DbusSettings::getClusterControllerMessagingParticipantId() const
{
    return settings.get<std::string>(DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID());
}

void DbusSettings::setClusterControllerMessagingParticipantId(const std::string& participantId)
{
    settings.set(DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID(), participantId);
}

std::string DbusSettings::createClusterControllerMessagingAddressString() const
{
    std::string address;
    address.append(getClusterControllerMessagingDomain());
    address += ':';
    address.append(getClusterControllerMessagingServiceName());
    address += ':';
    address.append(getClusterControllerMessagingParticipantId());
    return address;
}

void DbusSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger, "SETTING: {} = {}", SETTING_CC_MESSAGING_DOMAIN(),
                            settings.get<std::string>(SETTING_CC_MESSAGING_DOMAIN()));
    JOYNR_LOG_DEBUG(logger, "SETTING: {} = {}", SETTING_CC_MESSAGING_SERVICENAME(),
                            settings.get<std::string>(SETTING_CC_MESSAGING_SERVICENAME()));                
    JOYNR_LOG_DEBUG(logger, "SETTING: {} = {}", SETTING_CC_MESSAGING_PARTICIPANTID(),
                            settings.get<std::string>(SETTING_CC_MESSAGING_PARTICIPANTID()));
}

} // namespace joynr

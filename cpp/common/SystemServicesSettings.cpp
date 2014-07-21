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
#include "joynr/SystemServicesSettings.h"
#include <assert.h>
#include "joynr/SettingsMerger.h"

namespace joynr {

using namespace joynr_logging;

Logger* SystemServicesSettings::logger = Logging::getInstance()->getLogger("MSG", "SystemServicesSettings");

SystemServicesSettings::SystemServicesSettings(QSettings& settings, QObject* parent) :
        QObject(parent),
        settings(settings)
{
    QSettings defaultSystemServicesSettings(DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME(), QSettings::IniFormat);
    SettingsMerger::mergeSettings(defaultSystemServicesSettings, this->settings, false);
    checkSettings();
}

SystemServicesSettings::SystemServicesSettings(const SystemServicesSettings &other) :
        QObject(other.parent()),
        settings(other.settings)
{
}

SystemServicesSettings::~SystemServicesSettings() {
}

const QString &SystemServicesSettings::SETTING_DOMAIN()
{
    static const QString value("system.services/domain");
    return value;
}

const QString &SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()
{
    static const QString value("system.services/cc-routingprovider-authenticationtoken");
    return value;
}

const QString &SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()
{
    static const QString value("system.services/cc-routingprovider-participantid");
    return value;
}

const QString &SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()
{
    static const QString value("system.services/cc-discoveryprovider-authenticationtoken");
    return value;
}

const QString &SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()
{
    static const QString value("system.services/cc-discoveryprovider-participantid");
    return value;
}

const QString& SystemServicesSettings::DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME() {
    static const QString value("resources/default-system-services.settings");
    return value;
}

QString SystemServicesSettings::getDomain() const
{
    return settings.value(SETTING_DOMAIN()).toString();
}

void SystemServicesSettings::setJoynrSystemServicesDomain(const QString &systemServicesDomain)
{
    settings.setValue(SETTING_DOMAIN(), systemServicesDomain);
}

QString SystemServicesSettings::getCcRoutingProviderAuthenticationToken() const
{
    return settings.value(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()).toString();
}

void SystemServicesSettings::setCcRoutingProviderAuthenticationToken(const QString &authenticationToken)
{
    settings.setValue(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN(), authenticationToken);
}

QString SystemServicesSettings::getCcRoutingProviderParticipantId() const
{
    return settings.value(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()).toString();
}

void SystemServicesSettings::setCcRoutingProviderParticipantId(const QString &participantId)
{
    settings.setValue(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID(), participantId);
}

QString SystemServicesSettings::getCcDiscoveryProviderAuthenticationToken() const
{
    return settings.value(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()).toString();
}

void SystemServicesSettings::setCcDiscoveryProviderAuthenticationToken(const QString &authenticationToken)
{
    settings.setValue(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN(), authenticationToken);
}

QString SystemServicesSettings::getCcDiscoveryProviderParticipantId() const
{
    return settings.value(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()).toString();
}

void SystemServicesSettings::setCcDiscoveryProviderParticipantId(const QString &participantId)
{
    settings.setValue(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID(), participantId);
}

bool SystemServicesSettings::contains(const QString& key) const {
    return settings.contains(key);
}

QVariant SystemServicesSettings::value(const QString& key) const {
    return settings.value(key);
}

// Checks messaging settings and sets defaults
void SystemServicesSettings::checkSettings() const {
    assert(settings.contains(SETTING_DOMAIN()));
    assert(settings.contains(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()));
    assert(settings.contains(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()));
    assert(settings.contains(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()));
    assert(settings.contains(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()));
}


void SystemServicesSettings::printSettings() const {
    LOG_DEBUG(logger, "SETTING: " + SETTING_DOMAIN() + " = " + settings.value(SETTING_DOMAIN()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN() + " = " + settings.value(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID() + " = " + settings.value(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN() + " = " + settings.value(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID() + " = " + settings.value(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()).toString());
}


} // namespace joynr

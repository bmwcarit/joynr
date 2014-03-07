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
#include "DbusSettings.h"
#include <assert.h>
#include "joynr/SettingsMerger.h"

namespace joynr {

using namespace joynr_logging;

Logger* DbusSettings::logger = Logging::getInstance()->getLogger("MSG", "DbusSettings");

DbusSettings::DbusSettings(QSettings& settings, QObject *parent) :
    QObject(parent),
    settings(settings)
{
    QSettings defaultDbusSettings(DEFAULT_DBUS_SETTINGS_FILENAME(), QSettings::IniFormat);
    SettingsMerger::mergeSettings(defaultDbusSettings, this->settings, false);
    checkSettings();
}

DbusSettings::DbusSettings(const DbusSettings& other) :
    QObject(other.parent()),
    settings(other.settings)
{
}

DbusSettings::~DbusSettings () {
}

void DbusSettings::checkSettings() const {
    assert(settings.contains(SETTING_CC_MESSAGING_DOMAIN()));
    assert(settings.contains(SETTING_CC_MESSAGING_SERVICENAME()));
    assert(settings.contains(SETTING_CC_MESSAGING_PARTICIPANTID()));
    assert(settings.contains(SETTING_CC_CAPABILITIES_DOMAIN()));
    assert(settings.contains(SETTING_CC_CAPABILITIES_SERVICENAME()));
    assert(settings.contains(SETTING_CC_CAPABILITIES_PARTICIPANTID()));
}

const QString& DbusSettings::SETTING_CC_MESSAGING_DOMAIN() {
    static const QString value("dbus/cluster-controller-messaging-domain");
    return value;
}

const QString& DbusSettings::SETTING_CC_MESSAGING_SERVICENAME() {
    static const QString value("dbus/cluster-controller-messaging-servicename");
    return value;
}

const QString& DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID() {
    static const QString value("dbus/cluster-controller-messaging-participantid");
    return value;
}

const QString& DbusSettings::SETTING_CC_CAPABILITIES_DOMAIN() {
    static const QString value("dbus/cluster-controller-capabilities-domain");
    return value;
}

const QString& DbusSettings::SETTING_CC_CAPABILITIES_SERVICENAME() {
    static const QString value("dbus/cluster-controller-capabilities-servicename");
    return value;
}

const QString& DbusSettings::SETTING_CC_CAPABILITIES_PARTICIPANTID() {
    static const QString value("dbus/cluster-controller-capabilities-participantid");
    return value;
}

const QString& DbusSettings::DEFAULT_DBUS_SETTINGS_FILENAME() {
    static const QString value("resources/default-dbus.settings");
    return value;
}

QString DbusSettings::getClusterControllerMessagingDomain() const {
    return settings.value(DbusSettings::SETTING_CC_MESSAGING_DOMAIN()).toString();
}

void DbusSettings::setClusterControllerMessagingDomain(const QString& domain) {
    settings.setValue(DbusSettings::SETTING_CC_MESSAGING_DOMAIN(), domain);
}

QString DbusSettings::getClusterControllerMessagingServiceName() const {
    return settings.value(DbusSettings::SETTING_CC_MESSAGING_SERVICENAME()).toString();
}

void DbusSettings::setClusterControllerMessagingServiceName(const QString& serviceName) {
    settings.setValue(DbusSettings::SETTING_CC_MESSAGING_SERVICENAME(), serviceName);
}

QString DbusSettings::getClusterControllerMessagingParticipantId() const {
    return settings.value(DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID()).toString();
}

void DbusSettings::setClusterControllerMessagingParticipantId(const QString& participantId) {
    settings.setValue(DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID(), participantId);
}

QString DbusSettings::createClusterControllerMessagingAddressString() const {
    return QString("%1:%2:%3")
            .arg(getClusterControllerMessagingDomain())
            .arg(getClusterControllerMessagingServiceName())
            .arg(getClusterControllerMessagingParticipantId())
    ;
}

QString DbusSettings::createClusterControllerCapabilitiesAddressString() const {
    return QString("%1:%2:%3")
            .arg(getClusterControllerCapabilitiesDomain())
            .arg(getClusterControllerCapabilitiesServiceName())
            .arg(getClusterControllerCapabilitiesParticipantId())
    ;
}

QString DbusSettings::getClusterControllerCapabilitiesDomain() const {
    return settings.value(DbusSettings::SETTING_CC_CAPABILITIES_DOMAIN()).toString();
}

void DbusSettings::setClusterControllerCapabilitiesDomain(const QString& domain) {
    settings.setValue(DbusSettings::SETTING_CC_CAPABILITIES_DOMAIN(), domain);
}

QString DbusSettings::getClusterControllerCapabilitiesServiceName() const {
    return settings.value(DbusSettings::SETTING_CC_CAPABILITIES_SERVICENAME()).toString();
}

void DbusSettings::setClusterControllerCapabilitiesServiceName(const QString& serviceName) {
    settings.setValue(DbusSettings::SETTING_CC_CAPABILITIES_SERVICENAME(), serviceName);
}

QString DbusSettings::getClusterControllerCapabilitiesParticipantId() const {
    return settings.value(DbusSettings::SETTING_CC_CAPABILITIES_PARTICIPANTID()).toString();
}

void DbusSettings::setClusterControllerCapabilitiesParticipantId(const QString& participantId) {
    settings.setValue(DbusSettings::SETTING_CC_CAPABILITIES_PARTICIPANTID(), participantId);
}

bool DbusSettings::contains(const QString& key) const {
    return settings.contains(key);
}

QVariant DbusSettings::value(const QString& key) const {
    return settings.value(key);
}

void DbusSettings::printSettings() const {
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_MESSAGING_DOMAIN() + " = " + settings.value(SETTING_CC_MESSAGING_DOMAIN()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_MESSAGING_SERVICENAME() + " = " + settings.value(SETTING_CC_MESSAGING_SERVICENAME()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_MESSAGING_PARTICIPANTID() + " = " + settings.value(SETTING_CC_MESSAGING_PARTICIPANTID()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_CAPABILITIES_DOMAIN() + " = " + settings.value(SETTING_CC_CAPABILITIES_DOMAIN()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_CAPABILITIES_SERVICENAME() + " = " + settings.value(SETTING_CC_CAPABILITIES_SERVICENAME()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_CAPABILITIES_PARTICIPANTID() + " = " + settings.value(SETTING_CC_CAPABILITIES_PARTICIPANTID()).toString());
}

} // namespace joynr

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

namespace joynr {

using namespace joynr_logging;

Logger* DbusSettings::logger = Logging::getInstance()->getLogger("MSG", "DbusSettings");

DbusSettings::DbusSettings(QSettings& settings, QObject *parent) :
    QObject(parent),
    settings(settings)
{
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
    // set default values
    if(!settings.contains(DbusSettings::SETTING_CC_MESSAGING_ADDRESS())) {
        settings.setValue(DbusSettings::SETTING_CC_MESSAGING_ADDRESS(), "local:org.genivi.commonapi.joynr:cc.messaging");
    }
    if(!settings.contains(DbusSettings::SETTING_CC_CAPABILITIES_ADDRESS())) {
        settings.setValue(DbusSettings::SETTING_CC_CAPABILITIES_ADDRESS(), "local:org.genivi.commonapi.joynr:cc.capabilities");
    }
}

const QString& DbusSettings::SETTING_CC_MESSAGING_ADDRESS() {
    static const QString value("dbus/cluster-controller-messaging-address");
    return value;
}

const QString& DbusSettings::SETTING_CC_CAPABILITIES_ADDRESS() {
    static const QString value("dbus/cluster-controller-capabilities-address");
    return value;
}

QString DbusSettings::getClusterControllerMessagingAddress() const {
    return settings.value(DbusSettings::SETTING_CC_MESSAGING_ADDRESS()).toString();
}

void DbusSettings::setClusterControllerMessagingAddress(const QString& address) {
    settings.setValue(DbusSettings::SETTING_CC_MESSAGING_ADDRESS(), address);
}

QString DbusSettings::getClusterControllerCapabilitiesAddress() const {
    return settings.value(DbusSettings::SETTING_CC_CAPABILITIES_ADDRESS()).toString();
}

void DbusSettings::setClusterControllerCapabilitiesAddress(const QString& address) {
    settings.setValue(DbusSettings::SETTING_CC_CAPABILITIES_ADDRESS(), address);
}

void DbusSettings::printSettings() const {
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_MESSAGING_ADDRESS() + " = " + settings.value(SETTING_CC_MESSAGING_ADDRESS()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_CAPABILITIES_ADDRESS() + " = " + settings.value(SETTING_CC_CAPABILITIES_ADDRESS()).toString());
}

} // namespace joynr

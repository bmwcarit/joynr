/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include "WebSocketSettings.h"
#include <assert.h>
#include <QtCore/QUrl>
#include <QtCore/QMetaEnum>
#include "joynr/SettingsMerger.h"

namespace joynr {

using namespace joynr_logging;

Logger* WebSocketSettings::logger = Logging::getInstance()->getLogger("MSG", "WebSocketSettings");

WebSocketSettings::WebSocketSettings(QSettings& settings, QObject *parent) :
    QObject(parent),
    settings(settings)
{
    qRegisterMetaType<joynr::system::WebSocketAddress>("joynr::system::WebSocketAddress");
    qRegisterMetaType<joynr::system::WebSocketProtocol>();
    qRegisterMetaType<joynr::system::WebSocketProtocol::Enum>();
    QSettings defaultWebSocketSettings(DEFAULT_WEBSOCKET_SETTINGS_FILENAME(), QSettings::IniFormat);
    SettingsMerger::mergeSettings(defaultWebSocketSettings, this->settings, false);
    checkSettings();
}

WebSocketSettings::WebSocketSettings(const WebSocketSettings& other) :
    QObject(other.parent()),
    settings(other.settings)
{
}

WebSocketSettings::~WebSocketSettings () {
}

void WebSocketSettings::checkSettings() const {
    assert(settings.contains(SETTING_CC_MESSAGING_URL()));
}

const QString& WebSocketSettings::SETTING_CC_MESSAGING_URL() {
    static const QString value("websocket/cluster-controller-messaging-url");
    return value;
}

const QString& WebSocketSettings::DEFAULT_WEBSOCKET_SETTINGS_FILENAME() {
    static const QString value("resources/default-websocket.settings");
    return value;
}

QString WebSocketSettings::getClusterControllerMessagingUrl() const {
    return settings.value(WebSocketSettings::SETTING_CC_MESSAGING_URL()).toString();
}

void WebSocketSettings::setClusterControllerMessagingUrl(const QString& url) {
    settings.setValue(WebSocketSettings::SETTING_CC_MESSAGING_URL(), url);
}

joynr::system::WebSocketAddress WebSocketSettings::createClusterControllerMessagingAddress() const {
    QUrl url(getClusterControllerMessagingUrl());
    QMetaEnum metaEnum = joynr::system::WebSocketProtocol::staticMetaObject.enumerator(0);
    joynr::system::WebSocketProtocol::Enum protocol =
            (joynr::system::WebSocketProtocol::Enum) metaEnum.keyToValue(url.scheme().toUpper().toStdString().c_str());

    return system::WebSocketAddress(
                protocol,
                url.host(),
                url.port(),
                url.path()
    );
}

bool WebSocketSettings::contains(const QString& key) const {
    return settings.contains(key);
}

QVariant WebSocketSettings::value(const QString& key) const {
    return settings.value(key);
}

void WebSocketSettings::printSettings() const {
    LOG_DEBUG(logger, "SETTING: " + SETTING_CC_MESSAGING_URL() + " = " + settings.value(SETTING_CC_MESSAGING_URL()).toString());
}

} // namespace joynr

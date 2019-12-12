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
#include "joynr/WebSocketSettings.h"

#include <cassert>
#include <cstdint>

#include "joynr/Settings.h"
#include "joynr/Url.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketProtocol.h"

namespace joynr
{

WebSocketSettings::WebSocketSettings(Settings& settings) : _settings(settings)
{
    _settings.fillEmptySettingsWithDefaults(DEFAULT_WEBSOCKET_SETTINGS_FILENAME());
    checkSettings();
}

void WebSocketSettings::checkSettings()
{
    assert(_settings.contains(SETTING_CC_MESSAGING_URL()));
    assert(_settings.contains(SETTING_RECONNECT_SLEEP_TIME_MS()));

    if (!_settings.contains(SETTING_TLS_ENCRYPTION())) {
        setEncryptedTlsUsage(DEFAULT_TLS_ENCRYPTION());
    }
}

const std::string& WebSocketSettings::SETTING_CC_MESSAGING_URL()
{
    static const std::string value("websocket/cluster-controller-messaging-url");
    return value;
}

const std::string& WebSocketSettings::SETTING_RECONNECT_SLEEP_TIME_MS()
{
    static const std::string value("websocket/reconnect-sleep-time-ms");
    return value;
}

const std::string& WebSocketSettings::SETTING_CERTIFICATE_AUTHORITY_PEM_FILENAME()
{
    static const std::string value("websocket/certificate-authority-pem-filename");
    return value;
}

const std::string& WebSocketSettings::SETTING_CERTIFICATE_PEM_FILENAME()
{
    static const std::string value("websocket/certificate-pem-filename");
    return value;
}

const std::string& WebSocketSettings::SETTING_PRIVATE_KEY_PEM_FILENAME()
{
    static const std::string value("websocket/private-key-pem-filename");
    return value;
}

const std::string& WebSocketSettings::SETTING_TLS_ENCRYPTION()
{
    static const std::string value("websocket/tls-encryption");
    return value;
}

const std::string& WebSocketSettings::DEFAULT_WEBSOCKET_SETTINGS_FILENAME()
{
    static const std::string value("default-websocket.settings");
    return value;
}

bool WebSocketSettings::DEFAULT_TLS_ENCRYPTION()
{
    return true;
}

std::string WebSocketSettings::getClusterControllerMessagingUrl() const
{
    return _settings.get<std::string>(WebSocketSettings::SETTING_CC_MESSAGING_URL());
}

void WebSocketSettings::setClusterControllerMessagingUrl(const std::string& url)
{
    _settings.set(WebSocketSettings::SETTING_CC_MESSAGING_URL(), url);
}

joynr::system::RoutingTypes::WebSocketAddress WebSocketSettings::
        createClusterControllerMessagingAddress() const
{
    using joynr::system::RoutingTypes::WebSocketProtocol;

    std::string ccMessagingUrl = getClusterControllerMessagingUrl();
    Url url(ccMessagingUrl);

    if (!url.isValid()) {
        JOYNR_LOG_ERROR(logger(), "Could not parse URL: {}", ccMessagingUrl);
        return system::RoutingTypes::WebSocketAddress{};
    }

    WebSocketProtocol::Enum protocol = (url.getProtocol() == "wss") ? WebSocketProtocol::Enum::WSS
                                                                    : WebSocketProtocol::Enum::WS;

    return system::RoutingTypes::WebSocketAddress(
            protocol, url.getHost(), url.getPort(), url.getPath());
}

bool WebSocketSettings::getEncryptedTlsUsage() const
{
    return _settings.get<bool>(WebSocketSettings::SETTING_TLS_ENCRYPTION());
}

void WebSocketSettings::setEncryptedTlsUsage(bool encryptedTls)
{
    _settings.set(WebSocketSettings::SETTING_TLS_ENCRYPTION(), encryptedTls);
}

std::chrono::milliseconds WebSocketSettings::getReconnectSleepTimeMs() const
{
    return std::chrono::milliseconds(
            _settings.get<std::int64_t>(WebSocketSettings::SETTING_RECONNECT_SLEEP_TIME_MS()));
}

void WebSocketSettings::setReconnectSleepTimeMs(
        const std::chrono::milliseconds reconnectSleepTimeMs)
{
    _settings.set(
            WebSocketSettings::SETTING_RECONNECT_SLEEP_TIME_MS(), reconnectSleepTimeMs.count());
}

void WebSocketSettings::setCertificateAuthorityPemFilename(const std::string& filename)
{
    _settings.set(WebSocketSettings::SETTING_CERTIFICATE_AUTHORITY_PEM_FILENAME(), filename);
}

std::string WebSocketSettings::getCertificateAuthorityPemFilename() const
{
    return _settings.get<std::string>(
            WebSocketSettings::SETTING_CERTIFICATE_AUTHORITY_PEM_FILENAME());
}

void WebSocketSettings::setCertificatePemFilename(const std::string& filename)
{
    _settings.set(WebSocketSettings::SETTING_CERTIFICATE_PEM_FILENAME(), filename);
}

std::string WebSocketSettings::getCertificatePemFilename() const
{
    return _settings.get<std::string>(WebSocketSettings::SETTING_CERTIFICATE_PEM_FILENAME());
}

void WebSocketSettings::setPrivateKeyPemFilename(const std::string& filename)
{
    _settings.set(WebSocketSettings::SETTING_PRIVATE_KEY_PEM_FILENAME(), filename);
}

std::string WebSocketSettings::getPrivateKeyPemFilename() const
{
    return _settings.get<std::string>(WebSocketSettings::SETTING_PRIVATE_KEY_PEM_FILENAME());
}

bool WebSocketSettings::contains(const std::string& key) const
{
    return _settings.contains(key);
}

void WebSocketSettings::printSettings() const
{
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CC_MESSAGING_URL(),
                   _settings.get<std::string>(SETTING_CC_MESSAGING_URL()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CERTIFICATE_AUTHORITY_PEM_FILENAME(),
                   _settings.get<std::string>(SETTING_CERTIFICATE_AUTHORITY_PEM_FILENAME()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_CERTIFICATE_PEM_FILENAME(),
                   _settings.get<std::string>(SETTING_CERTIFICATE_PEM_FILENAME()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_PRIVATE_KEY_PEM_FILENAME(),
                   _settings.get<std::string>(SETTING_PRIVATE_KEY_PEM_FILENAME()));

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_TLS_ENCRYPTION(),
                   _settings.get<std::string>(SETTING_TLS_ENCRYPTION()));
}

} // namespace joynr

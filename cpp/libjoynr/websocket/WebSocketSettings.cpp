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

#include <cassert>

#include "joynr/Settings.h"
#include "joynr/Url.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

namespace joynr
{

INIT_LOGGER(WebSocketSettings);

WebSocketSettings::WebSocketSettings(Settings& settings) : settings(settings)
{
    settings.fillEmptySettingsWithDefaults(DEFAULT_WEBSOCKET_SETTINGS_FILENAME());
    checkSettings();
}

void WebSocketSettings::checkSettings() const
{
    assert(settings.contains(SETTING_CC_MESSAGING_URL()));
    assert(settings.contains(SETTING_RECONNECT_SLEEP_TIME_MS()));
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

const std::string& WebSocketSettings::DEFAULT_WEBSOCKET_SETTINGS_FILENAME()
{
    static const std::string value("default-websocket.settings");
    return value;
}

std::string WebSocketSettings::getClusterControllerMessagingUrl() const
{
    return settings.get<std::string>(WebSocketSettings::SETTING_CC_MESSAGING_URL());
}

void WebSocketSettings::setClusterControllerMessagingUrl(const std::string& url)
{
    settings.set(WebSocketSettings::SETTING_CC_MESSAGING_URL(), url);
}

joynr::system::RoutingTypes::WebSocketAddress WebSocketSettings::
        createClusterControllerMessagingAddress() const
{
    using joynr::system::RoutingTypes::WebSocketProtocol;

    std::string ccMessagingUrl = getClusterControllerMessagingUrl();
    Url url(ccMessagingUrl);

    if (!url.isValid()) {
        JOYNR_LOG_ERROR(logger, "Could not parse URL: {}", ccMessagingUrl);
        return system::RoutingTypes::WebSocketAddress{};
    }

    WebSocketProtocol::Enum protocol = (url.getProtocol() == "wss") ? WebSocketProtocol::Enum::WSS
                                                                    : WebSocketProtocol::Enum::WS;

    return system::RoutingTypes::WebSocketAddress(
            protocol, url.getHost(), url.getPort(), url.getPath());
}

std::chrono::milliseconds WebSocketSettings::getReconnectSleepTimeMs() const
{
    return std::chrono::milliseconds(
            settings.get<std::int64_t>(WebSocketSettings::SETTING_RECONNECT_SLEEP_TIME_MS()));
}

void WebSocketSettings::setReconnectSleepTimeMs(
        const std::chrono::milliseconds reconnectSleepTimeMs)
{
    settings.set(
            WebSocketSettings::SETTING_RECONNECT_SLEEP_TIME_MS(), reconnectSleepTimeMs.count());
}

bool WebSocketSettings::contains(const std::string& key) const
{
    return settings.contains(key);
}

void WebSocketSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_CC_MESSAGING_URL(),
                    settings.get<std::string>(SETTING_CC_MESSAGING_URL()));
}

} // namespace joynr

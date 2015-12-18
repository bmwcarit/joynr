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
#include "joynr/MessagingSettings.h"
#include "joynr/BounceProxyUrl.h"
#include "joynr/TypeUtil.h"
#include "joynr/Settings.h"

#include <cassert>

namespace joynr
{

using namespace joynr_logging;

Logger* MessagingSettings::logger = Logging::getInstance()->getLogger("MSG", "MessagingSettings");

MessagingSettings::MessagingSettings(Settings& settings) : settings(settings)
{
    Settings defaultMessagingSettings{DEFAULT_MESSAGING_SETTINGS_FILENAME()};
    Settings::merge(defaultMessagingSettings, this->settings, false);
    checkSettings();
}

MessagingSettings::MessagingSettings(const MessagingSettings& other) : settings(other.settings)
{
}

MessagingSettings::~MessagingSettings()
{
}

const std::string& MessagingSettings::SETTING_BOUNCE_PROXY_URL()
{
    static const std::string value("messaging/bounce-proxy-url");
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()
{
    static const std::string value("messaging/discovery-directories-domain");
    return value;
}

const std::string& MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_URL()
{
    static const std::string value("messaging/channel-url-directory-url");
    return value;
}

const std::string& MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()
{
    static const std::string value("messaging/channel-url-directory-channelid");
    return value;
}

const std::string& MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()
{
    static const std::string value("messaging/channel-url-directory-participantid");
    return value;
}

const std::string& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_URL()
{
    static const std::string value("messaging/capabilities-directory-url");
    return value;
}

const std::string& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()
{
    static const std::string value("messaging/capabilities-directory-channelid");
    return value;
}

const std::string& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()
{
    static const std::string value("messaging/capabilities-directory-participantid");
    return value;
}

const std::string& MessagingSettings::SETTING_CERTIFICATE_AUTHORITY()
{
    static const std::string value("messaging/certificate-authority");
    return value;
}

const std::string& MessagingSettings::SETTING_CLIENT_CERTIFICATE()
{
    static const std::string value("messaging/client-certificate");
    return value;
}

const std::string& MessagingSettings::SETTING_CLIENT_CERTIFICATE_PASSWORD()
{
    static const std::string value("messaging/client-certificate-password");
    return value;
}

const std::string& MessagingSettings::SETTING_INDEX()
{
    static const std::string value("messaging/index");
    return value;
}

const std::string& MessagingSettings::SETTING_CREATE_CHANNEL_RETRY_INTERVAL()
{
    static const std::string value("messaging/create-channel-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_DELETE_CHANNEL_RETRY_INTERVAL()
{
    static const std::string value("messaging/delete-channel-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_SEND_MSG_RETRY_INTERVAL()
{
    static const std::string value("messaging/send-msg-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_LONGPOLL_RETRY_INTERVAL()
{
    static const std::string value("messaging/longpoll-retry-interval");
    return value;
}

const std::string& MessagingSettings::SETTING_LOCAL_PROXY_HOST()
{
    static const std::string value("messaging/local-proxy-host");
    return value;
}

const std::string& MessagingSettings::SETTING_HTTP_DEBUG()
{
    static const std::string value("messaging/http-debug");
    return value;
}

const std::string& MessagingSettings::SETTING_LOCAL_PROXY_PORT()
{
    static const std::string value("messaging/local-proxy-port");
    return value;
}

const std::string& MessagingSettings::SETTING_PERSISTENCE_FILENAME()
{
    static const std::string value("messaging/persistence-file");
    return value;
}

std::string MessagingSettings::getCertificateAuthority() const
{
    return settings.get<std::string>(SETTING_CERTIFICATE_AUTHORITY());
}

void MessagingSettings::setCertificateAuthority(const std::string& certificateAuthority)
{
    settings.set(SETTING_CERTIFICATE_AUTHORITY(), certificateAuthority);
}

std::string MessagingSettings::getClientCertificate() const
{
    return settings.get<std::string>(SETTING_CLIENT_CERTIFICATE());
}

void MessagingSettings::setClientCertificate(const std::string& clientCertificate)
{
    settings.set(SETTING_CLIENT_CERTIFICATE(), clientCertificate);
}

std::string MessagingSettings::getClientCertificatePassword() const
{
    return settings.get<std::string>(SETTING_CLIENT_CERTIFICATE_PASSWORD());
}

void MessagingSettings::setClientCertificatePassword(const std::string& clientCertificatePassword)
{
    settings.set(SETTING_CLIENT_CERTIFICATE_PASSWORD(), clientCertificatePassword);
}

const std::string& MessagingSettings::DEFAULT_MESSAGING_SETTINGS_FILENAME()
{
    static const std::string value("resources/default-messaging.settings");
    return value;
}

const std::string& MessagingSettings::DEFAULT_PERSISTENCE_FILENAME()
{
    static const std::string value("joynr.settings");
    return value;
}

const std::string& MessagingSettings::SETTING_LONGPOLL_TIMEOUT_MS()
{
    static const std::string value("messaging/long-poll-timeout");
    return value;
}

int64_t MessagingSettings::DEFAULT_LONGPOLL_TIMEOUT_MS()
{
    static const int64_t value(10 * 60 * 1000); // 10 minutes
    return value;
}

const std::string& MessagingSettings::SETTING_HTTP_CONNECT_TIMEOUT_MS()
{
    static const std::string value("messaging/http-connect-timeout");
    return value;
}

int64_t MessagingSettings::DEFAULT_HTTP_CONNECT_TIMEOUT_MS()
{
    static const int64_t value(1 * 60 * 1000); // 1 minute
    return value;
}

const std::string& MessagingSettings::SETTING_BOUNCEPROXY_TIMEOUT_MS()
{
    static const std::string value("messaging/bounce-proxy-timeout");
    return value;
}

int64_t MessagingSettings::DEFAULT_BOUNCEPROXY_TIMEOUT_MS()
{
    static const int64_t value(20 * 1000); // 20 seconds
    return value;
}

const std::string& MessagingSettings::SETTING_DISCOVERY_MESSAGES_TTL_MS()
{
    static const std::string value("messaging/discovery-messages-ttl");
    return value;
}

int64_t MessagingSettings::DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS()
{
    static const int64_t value(40 * 1000); // 40 seconds
    return value;
}

const std::string& MessagingSettings::SETTING_SEND_MESSAGE_MAX_TTL()
{
    static const std::string value("messaging/max-send-ttl");
    return value;
}

int64_t MessagingSettings::DEFAULT_SEND_MESSAGE_MAX_TTL()
{
    static const int64_t value(10 * 60 * 1000); // 10 minutes
    return value;
}

BounceProxyUrl MessagingSettings::getBounceProxyUrl() const
{
    return BounceProxyUrl(settings.get<std::string>(SETTING_BOUNCE_PROXY_URL()));
}

std::string MessagingSettings::getBounceProxyUrlString() const
{
    return settings.get<std::string>(SETTING_BOUNCE_PROXY_URL());
}

void MessagingSettings::setBounceProxyUrl(const BounceProxyUrl& bounceProxyUrl)
{
    std::string url = bounceProxyUrl.getBounceProxyBaseUrl().toString().toStdString();
    settings.set(SETTING_BOUNCE_PROXY_URL(), url);
}

std::string MessagingSettings::getDiscoveryDirectoriesDomain() const
{
    return settings.get<std::string>(SETTING_DISCOVERY_DIRECTORIES_DOMAIN());
}

std::string MessagingSettings::getChannelUrlDirectoryUrl() const
{
    return settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_URL());
}

std::string MessagingSettings::getChannelUrlDirectoryChannelId() const
{
    return settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID());
}

std::string MessagingSettings::getChannelUrlDirectoryParticipantId() const
{
    return settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID());
}

std::string MessagingSettings::getCapabilitiesDirectoryUrl() const
{
    return settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_URL());
}

std::string MessagingSettings::getCapabilitiesDirectoryChannelId() const
{
    return settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_CHANNELID());
}

std::string MessagingSettings::getCapabilitiesDirectoryParticipantId() const
{
    return settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID());
}

int64_t MessagingSettings::getIndex() const
{
    return settings.get<int64_t>(SETTING_INDEX());
}

void MessagingSettings::setIndex(int64_t index)
{
    settings.set(SETTING_INDEX(), index);
}

int MessagingSettings::getCreateChannelRetryInterval() const
{
    return settings.get<int>(SETTING_CREATE_CHANNEL_RETRY_INTERVAL());
}

void MessagingSettings::setCreateChannelRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_CREATE_CHANNEL_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getDeleteChannelRetryInterval() const
{
    return settings.get<int>(SETTING_DELETE_CHANNEL_RETRY_INTERVAL());
}

void MessagingSettings::setDeleteChannelRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_DELETE_CHANNEL_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getSendMsgRetryInterval() const
{
    return settings.get<int>(SETTING_SEND_MSG_RETRY_INTERVAL());
}

void MessagingSettings::setSendMsgRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_SEND_MSG_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getLongPollRetryInterval() const
{
    return settings.get<int>(SETTING_LONGPOLL_RETRY_INTERVAL());
}

void MessagingSettings::setLongPollRetryInterval(const int& retryInterval)
{
    settings.set(SETTING_LONGPOLL_RETRY_INTERVAL(), retryInterval);
}

std::string MessagingSettings::getLocalProxyPort() const
{
    return settings.get<std::string>(SETTING_LOCAL_PROXY_PORT());
}

void MessagingSettings::setLocalProxyPort(const int& localProxyPort)
{
    settings.set(SETTING_LOCAL_PROXY_PORT(), localProxyPort);
}

std::string MessagingSettings::getLocalProxyHost() const
{
    return settings.get<std::string>(SETTING_LOCAL_PROXY_HOST());
}

void MessagingSettings::setLocalProxyHost(const std::string& localProxyHost)
{
    settings.set(SETTING_LOCAL_PROXY_HOST(), localProxyHost);
}

bool MessagingSettings::getHttpDebug() const
{
    return settings.get<bool>(SETTING_HTTP_DEBUG());
}

void MessagingSettings::setHttpDebug(const bool& httpDebug)
{
    settings.set(SETTING_HTTP_DEBUG(), httpDebug);
}

std::string MessagingSettings::getMessagingPropertiesPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_PERSISTENCE_FILENAME());
}

void MessagingSettings::setMessagingPropertiesPersistenceFilename(const std::string& filename)
{
    settings.set(SETTING_PERSISTENCE_FILENAME(), filename);
}

int64_t MessagingSettings::getLongPollTimeout() const
{
    return settings.get<int64_t>(SETTING_LONGPOLL_TIMEOUT_MS());
}

void MessagingSettings::setLongPollTimeout(int64_t timeout_ms)
{
    settings.set(SETTING_LONGPOLL_TIMEOUT_MS(), timeout_ms);
}

int64_t MessagingSettings::getHttpConnectTimeout() const
{
    return settings.get<int64_t>(SETTING_HTTP_CONNECT_TIMEOUT_MS());
}

void MessagingSettings::setHttpConnectTimeout(int64_t timeout_ms)
{
    settings.set(SETTING_HTTP_CONNECT_TIMEOUT_MS(), timeout_ms);
}

int64_t MessagingSettings::getBounceProxyTimeout() const
{
    return settings.get<int64_t>(SETTING_BOUNCEPROXY_TIMEOUT_MS());
}

void MessagingSettings::setBounceProxyTimeout(int64_t timeout_ms)
{
    settings.set(SETTING_BOUNCEPROXY_TIMEOUT_MS(), timeout_ms);
}

int64_t MessagingSettings::getDiscoveryMessagesTtl() const
{
    return settings.get<int64_t>(SETTING_DISCOVERY_MESSAGES_TTL_MS());
}

void MessagingSettings::setDiscoveryMessagesTtl(int64_t ttl_ms)
{
    settings.set(SETTING_DISCOVERY_MESSAGES_TTL_MS(), ttl_ms);
}

int64_t MessagingSettings::getSendMsgMaxTtl() const
{
    return settings.get<int64_t>(SETTING_SEND_MESSAGE_MAX_TTL());
}

void MessagingSettings::setSendMsgMaxTtl(int64_t ttl_ms)
{
    settings.set(SETTING_SEND_MESSAGE_MAX_TTL(), ttl_ms);
}

bool MessagingSettings::contains(const std::string& key) const
{
    return settings.contains(key);
}

// Checks messaging settings and sets defaults
void MessagingSettings::checkSettings() const
{
    assert(settings.contains(SETTING_BOUNCE_PROXY_URL()));
    std::string bounceProxyUrl = settings.get<std::string>(SETTING_BOUNCE_PROXY_URL());
    if (bounceProxyUrl.back() != '/') {
        bounceProxyUrl.append("/");
        settings.set(SETTING_BOUNCE_PROXY_URL(), bounceProxyUrl);
    }

    assert(settings.contains(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    assert(settings.contains(SETTING_CHANNEL_URL_DIRECTORY_URL()));
    std::string channelUrlDirectoryUrl =
            settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_URL());
    if (channelUrlDirectoryUrl.back() != '/') {
        channelUrlDirectoryUrl.append("/");
        settings.set(SETTING_CHANNEL_URL_DIRECTORY_URL(), channelUrlDirectoryUrl);
    }
    assert(settings.contains(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()));
    assert(settings.contains(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()));

    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_URL()));
    std::string capabilitiesDirectoryUrl =
            settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_URL());
    if (capabilitiesDirectoryUrl.back() != '/') {
        capabilitiesDirectoryUrl.append("/");
        settings.set(SETTING_CAPABILITIES_DIRECTORY_URL(), capabilitiesDirectoryUrl);
    }
    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    if (!settings.contains(SETTING_INDEX())) {
        settings.set(SETTING_INDEX(), 0);
    }
    if (!settings.contains(SETTING_CREATE_CHANNEL_RETRY_INTERVAL())) {
        settings.set(SETTING_CREATE_CHANNEL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_DELETE_CHANNEL_RETRY_INTERVAL())) {
        settings.set(SETTING_DELETE_CHANNEL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_SEND_MSG_RETRY_INTERVAL())) {
        settings.set(SETTING_SEND_MSG_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_LONGPOLL_RETRY_INTERVAL())) {
        settings.set(SETTING_LONGPOLL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_PERSISTENCE_FILENAME())) {
        settings.set(SETTING_PERSISTENCE_FILENAME(), DEFAULT_PERSISTENCE_FILENAME());
    }
    if (!settings.contains(SETTING_DISCOVERY_MESSAGES_TTL_MS())) {
        settings.set(SETTING_DISCOVERY_MESSAGES_TTL_MS(), DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS());
    }
}

void MessagingSettings::printSettings() const
{
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_BOUNCE_PROXY_URL())
                      .arg(settings.get<std::string>(SETTING_BOUNCE_PROXY_URL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_DISCOVERY_DIRECTORIES_DOMAIN())
                      .arg(settings.get<std::string>(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_CHANNEL_URL_DIRECTORY_URL())
                      .arg(settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_URL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID())
                      .arg(settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID())
                      .arg(settings.get<std::string>(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_CAPABILITIES_DIRECTORY_URL())
                      .arg(settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_URL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_CAPABILITIES_DIRECTORY_CHANNELID())
                      .arg(settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()))
                      .str());
    LOG_DEBUG(
            logger,
            FormatString("SETTING: %1 = %2")
                    .arg(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID())
                    .arg(settings.get<std::string>(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()))
                    .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_INDEX())
                      .arg(settings.get<std::string>(SETTING_INDEX()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_CREATE_CHANNEL_RETRY_INTERVAL())
                      .arg(settings.get<std::string>(SETTING_CREATE_CHANNEL_RETRY_INTERVAL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_DELETE_CHANNEL_RETRY_INTERVAL())
                      .arg(settings.get<std::string>(SETTING_DELETE_CHANNEL_RETRY_INTERVAL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_SEND_MSG_RETRY_INTERVAL())
                      .arg(settings.get<std::string>(SETTING_SEND_MSG_RETRY_INTERVAL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_LONGPOLL_RETRY_INTERVAL())
                      .arg(settings.get<std::string>(SETTING_LONGPOLL_RETRY_INTERVAL()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_LOCAL_PROXY_HOST())
                      .arg(settings.get<std::string>(SETTING_LOCAL_PROXY_HOST()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_LOCAL_PROXY_PORT())
                      .arg(settings.get<std::string>(SETTING_LOCAL_PROXY_PORT()))
                      .str());
    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_PERSISTENCE_FILENAME())
                      .arg(settings.get<std::string>(SETTING_PERSISTENCE_FILENAME()))
                      .str());

    LOG_DEBUG(logger,
              FormatString("SETTING: %1 = %2")
                      .arg(SETTING_DISCOVERY_MESSAGES_TTL_MS())
                      .arg(settings.get<std::string>(SETTING_DISCOVERY_MESSAGES_TTL_MS()))
                      .str());
}

} // namespace joynr

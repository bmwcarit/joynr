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
#include <assert.h>
#include "joynr/BounceProxyUrl.h"
#include "joynr/SettingsMerger.h"

namespace joynr {

using namespace joynr_logging;

Logger* MessagingSettings::logger = Logging::getInstance()->getLogger("MSG", "MessagingSettings");

MessagingSettings::MessagingSettings(QSettings& settings, QObject* parent) :
        QObject(parent),
        settings(settings)
{
    QSettings defaultMessagingSettings(DEFAULT_MESSAGING_SETTINGS_FILENAME(), QSettings::IniFormat);
    SettingsMerger::mergeSettings(defaultMessagingSettings, this->settings, false);
    checkSettings();
}

MessagingSettings::MessagingSettings(const MessagingSettings &other) :
        QObject(other.parent()),
        settings(other.settings)
{
}

MessagingSettings::~MessagingSettings() {
}

const QString& MessagingSettings::SETTING_BOUNCE_PROXY_URL() {
    static const QString value("messaging/bounce-proxy-url");
    return value;
}

const QString& MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN() {
    static const QString value("messaging/discovery-directories-domain");
    return value;
}

const QString& MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_URL() {
    static const QString value("messaging/channel-url-directory-url");
    return value;
}

const QString& MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_CHANNELID() {
    static const QString value("messaging/channel-url-directory-channelid");
    return value;
}

const QString& MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID() {
    static const QString value("messaging/channel-url-directory-participantid");
    return value;
}

const QString& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_URL() {
    static const QString value("messaging/capabilities-directory-url");
    return value;
}

const QString& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID() {
    static const QString value("messaging/capabilities-directory-channelid");
    return value;
}

const QString& MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID() {
    static const QString value("messaging/capabilities-directory-participantid");
    return value;
}

const QString& MessagingSettings::SETTING_INDEX() {
    static const QString value("messaging/index");
    return value;
}

const QString& MessagingSettings::SETTING_CREATE_CHANNEL_RETRY_INTERVAL() {
    static const QString value("messaging/create-channel-retry-interval");
    return value;
}

const QString& MessagingSettings::SETTING_DELETE_CHANNEL_RETRY_INTERVAL() {
    static const QString value("messaging/delete-channel-retry-interval");
    return value;
}

const QString& MessagingSettings::SETTING_SEND_MSG_RETRY_INTERVAL() {
    static const QString value("messaging/send-msg-retry-interval");
    return value;
}

const QString& MessagingSettings::SETTING_LONGPOLL_RETRY_INTERVAL() {
    static const QString value("messaging/longpoll-retry-interval");
    return value;
}

const QString& MessagingSettings::SETTING_LOCAL_PROXY_HOST() {
    static const QString value("messaging/local-proxy-host");
    return value;
}

const QString& MessagingSettings::SETTING_HTTP_DEBUG() {
    static const QString value("messaging/http-debug");
    return value;
}

const QString& MessagingSettings::SETTING_LOCAL_PROXY_PORT() {
    static const QString value("messaging/local-proxy-port");
    return value;
}

const QString& MessagingSettings::SETTING_PERSISTENCE_FILENAME() {
    static const QString value("messaging/persistence-file");
    return value;
}

const QString& MessagingSettings::DEFAULT_MESSAGING_SETTINGS_FILENAME() {
    static const QString value("resources/default-messaging.settings");
    return value;
}

const QString& MessagingSettings::DEFAULT_PERSISTENCE_FILENAME() {
    static const QString value("joynr.settings");
    return value;
}

const QString& MessagingSettings::SETTING_LONGPOLL_TIMEOUT_MS() {
    static const QString value("messaging/long-poll-timeout");
    return value;
}

qint64 MessagingSettings::DEFAULT_LONGPOLL_TIMEOUT_MS() {
    static const qint64 value(10 * 60 * 1000);  // 10 minutes
    return value;
}

const QString& MessagingSettings::SETTING_HTTP_CONNECT_TIMEOUT_MS() {
    static const QString value("messaging/http-connect-timeout");
    return value;
}

qint64 MessagingSettings::DEFAULT_HTTP_CONNECT_TIMEOUT_MS() {
    static const qint64 value(1 * 60 * 1000);  // 1 minute
    return value;
}

const QString& MessagingSettings::SETTING_BOUNCEPROXY_TIMEOUT_MS() {
    static const QString value("messaging/bounce-proxy-timeout");
    return value;
}

qint64 MessagingSettings::DEFAULT_BOUNCEPROXY_TIMEOUT_MS() {
    static const qint64 value(20 * 1000);  // 20 seconds
    return value;
}

const QString& MessagingSettings::SETTING_SEND_MESSAGE_MAX_TTL() {
    static const QString value("messaging/max-send-ttl");
    return value;
}

qint64 MessagingSettings::DEFAULT_SEND_MESSAGE_MAX_TTL() {
    static const qint64 value(10 * 60 * 1000);  // 10 minutes
    return value;
}

BounceProxyUrl MessagingSettings::getBounceProxyUrl() const {
    return BounceProxyUrl(settings.value(SETTING_BOUNCE_PROXY_URL()).toString());
}

void MessagingSettings::setBounceProxyUrl(const BounceProxyUrl& bounceProxyUrl) {
    settings.setValue(SETTING_BOUNCE_PROXY_URL(), bounceProxyUrl.getBounceProxyBaseUrl());
}

QString MessagingSettings::getDiscoveryDirectoriesDomain() const {
    return settings.value(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()).toString();
}

QString MessagingSettings::getChannelUrlDirectoryUrl() const {
    return settings.value(SETTING_CHANNEL_URL_DIRECTORY_URL()).toString();
}

QString MessagingSettings::getChannelUrlDirectoryChannelId() const {
    return settings.value(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()).toString();
}

QString MessagingSettings::getChannelUrlDirectoryParticipantId() const {
    return settings.value(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()).toString();
}

QString MessagingSettings::getCapabilitiesDirectoryUrl() const {
    return settings.value(SETTING_CAPABILITIES_DIRECTORY_URL()).toString();
}

QString MessagingSettings::getCapabilitiesDirectoryChannelId() const {
    return settings.value(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()).toString();
}

QString MessagingSettings::getCapabilitiesDirectoryParticipantId() const {
    return settings.value(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()).toString();
}

qint64 MessagingSettings::getIndex() const {
    return settings.value(SETTING_INDEX()).toLongLong();
}

void MessagingSettings::setIndex(qint64 index) {
    settings.setValue(SETTING_INDEX(), index);
}

int MessagingSettings::getCreateChannelRetryInterval() const {
    return settings.value(SETTING_CREATE_CHANNEL_RETRY_INTERVAL()).toInt();
}

void MessagingSettings::setCreateChannelRetryInterval(const int &retryInterval) {
    settings.setValue(SETTING_CREATE_CHANNEL_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getDeleteChannelRetryInterval() const {
    return settings.value(SETTING_DELETE_CHANNEL_RETRY_INTERVAL()).toInt();
}

void MessagingSettings::setDeleteChannelRetryInterval(const int &retryInterval) {
    settings.setValue(SETTING_DELETE_CHANNEL_RETRY_INTERVAL(), retryInterval);
}


int MessagingSettings::getSendMsgRetryInterval() const {
    return settings.value(SETTING_SEND_MSG_RETRY_INTERVAL()).toInt();
}

void MessagingSettings::setSendMsgRetryInterval(const int &retryInterval) {
    settings.setValue(SETTING_SEND_MSG_RETRY_INTERVAL(), retryInterval);
}

int MessagingSettings::getLongPollRetryInterval() const {
    return settings.value(SETTING_LONGPOLL_RETRY_INTERVAL()).toInt();
}

void MessagingSettings::setLongPollRetryInterval(const int &retryInterval) {
    settings.setValue(SETTING_LONGPOLL_RETRY_INTERVAL(), retryInterval);
}

QString MessagingSettings::getLocalProxyPort() const {
    return settings.value(SETTING_LOCAL_PROXY_PORT()).toString();
}

void MessagingSettings::setLocalProxyPort(const int &localProxyPort) {
    settings.setValue(SETTING_LOCAL_PROXY_PORT(), localProxyPort);
}

QString MessagingSettings::getLocalProxyHost() const {
    return settings.value(SETTING_LOCAL_PROXY_HOST()).toString();
}

void MessagingSettings::setLocalProxyHost(const QString &localProxyHost) {
    settings.setValue(SETTING_LOCAL_PROXY_HOST(), localProxyHost);
}

bool MessagingSettings::getHttpDebug() const {
    return settings.value(SETTING_HTTP_DEBUG()).toBool();
}

void MessagingSettings::setHttpDebug(const bool& httpDebug) {
    settings.setValue(SETTING_HTTP_DEBUG(), httpDebug);
}

QString MessagingSettings::getMessagingPropertiesPersistenceFilename() const {
    return settings.value(SETTING_PERSISTENCE_FILENAME()).toString();
}

void MessagingSettings::setMessagingPropertiesPersistenceFilename(const QString& filename) {
    settings.setValue(SETTING_PERSISTENCE_FILENAME(), filename);
}

qint64 MessagingSettings::getLongPollTimeout() const {
    return settings.value(SETTING_LONGPOLL_TIMEOUT_MS()).toLongLong();
}

void MessagingSettings::setLongPollTimeout(qint64 timeout_ms) {
    settings.setValue(SETTING_LONGPOLL_TIMEOUT_MS(), timeout_ms);
}

qint64 MessagingSettings::getHttpConnectTimeout() const {
    return settings.value(SETTING_HTTP_CONNECT_TIMEOUT_MS()).toLongLong();
}

void MessagingSettings::setHttpConnectTimeout(qint64 timeout_ms) {
    settings.setValue(SETTING_HTTP_CONNECT_TIMEOUT_MS(), timeout_ms);
}

qint64 MessagingSettings::getBounceProxyTimeout() const {
    return settings.value(SETTING_BOUNCEPROXY_TIMEOUT_MS()).toLongLong();
}

void MessagingSettings::setBounceProxyTimeout(qint64 timeout_ms) {
    settings.setValue(SETTING_BOUNCEPROXY_TIMEOUT_MS(), timeout_ms);
}

qint64 MessagingSettings::getSendMsgMaxTtl() const {
    return settings.value(SETTING_SEND_MESSAGE_MAX_TTL()).toLongLong();
}

void MessagingSettings::setSendMsgMaxTtl(qint64 ttl_ms) {
    settings.setValue(SETTING_SEND_MESSAGE_MAX_TTL(), ttl_ms);
}

bool MessagingSettings::contains(const QString& key) const {
    return settings.contains(key);
}

QVariant MessagingSettings::value(const QString& key) const {
    return settings.value(key);
}

// Checks messaging settings and sets defaults
void MessagingSettings::checkSettings() const {
    assert(settings.contains(SETTING_BOUNCE_PROXY_URL()));
    QString bounceProxyUrl = settings.value(SETTING_BOUNCE_PROXY_URL()).toString();
    if (!bounceProxyUrl.endsWith("/")) {
        bounceProxyUrl.append("/");
        settings.setValue(SETTING_BOUNCE_PROXY_URL(), bounceProxyUrl);
    }

    assert(settings.contains(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    assert(settings.contains(SETTING_CHANNEL_URL_DIRECTORY_URL()));
    QString channelUrlDirectoryUrl = settings.value(SETTING_CHANNEL_URL_DIRECTORY_URL()).toString();
    if (!channelUrlDirectoryUrl.endsWith("/")) {
        channelUrlDirectoryUrl.append("/");
        settings.setValue(SETTING_CHANNEL_URL_DIRECTORY_URL(), channelUrlDirectoryUrl);
    }
    assert(settings.contains(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()));
    assert(settings.contains(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()));

    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_URL()));
    QString capabilitiesDirectoryUrl = settings.value(SETTING_CAPABILITIES_DIRECTORY_URL()).toString();
    if (!capabilitiesDirectoryUrl.endsWith("/")) {
        capabilitiesDirectoryUrl.append("/");
        settings.setValue(SETTING_CAPABILITIES_DIRECTORY_URL(), capabilitiesDirectoryUrl);
    }
    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    assert(settings.contains(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    if (!settings.contains(SETTING_INDEX())) {
        settings.setValue(SETTING_INDEX(), 0);
    }
    if (!settings.contains(SETTING_CREATE_CHANNEL_RETRY_INTERVAL())) {
        settings.setValue(SETTING_CREATE_CHANNEL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_DELETE_CHANNEL_RETRY_INTERVAL())) {
        settings.setValue(SETTING_DELETE_CHANNEL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_SEND_MSG_RETRY_INTERVAL())) {
        settings.setValue(SETTING_SEND_MSG_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_LONGPOLL_RETRY_INTERVAL())) {
        settings.setValue(SETTING_LONGPOLL_RETRY_INTERVAL(), 5000);
    }
    if (!settings.contains(SETTING_PERSISTENCE_FILENAME())) {
        settings.setValue(SETTING_PERSISTENCE_FILENAME(),
                           DEFAULT_PERSISTENCE_FILENAME());
    }
}


void MessagingSettings::printSettings() const {
    LOG_DEBUG(logger, "SETTING: " + SETTING_BOUNCE_PROXY_URL() + " = " + settings.value(SETTING_BOUNCE_PROXY_URL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_DISCOVERY_DIRECTORIES_DOMAIN() + " = " + settings.value(SETTING_DISCOVERY_DIRECTORIES_DOMAIN()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CHANNEL_URL_DIRECTORY_URL() + " = " + settings.value(SETTING_CHANNEL_URL_DIRECTORY_URL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CHANNEL_URL_DIRECTORY_CHANNELID() + " = " + settings.value(SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID() + " = " + settings.value(SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CAPABILITIES_DIRECTORY_URL() + " = " + settings.value(SETTING_CAPABILITIES_DIRECTORY_URL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CAPABILITIES_DIRECTORY_CHANNELID() + " = " + settings.value(SETTING_CAPABILITIES_DIRECTORY_CHANNELID()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID() + " = " + settings.value(SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_INDEX() + " = " + settings.value(SETTING_INDEX()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_CREATE_CHANNEL_RETRY_INTERVAL() + " = " + settings.value(SETTING_CREATE_CHANNEL_RETRY_INTERVAL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_DELETE_CHANNEL_RETRY_INTERVAL() + " = " + settings.value(SETTING_DELETE_CHANNEL_RETRY_INTERVAL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_SEND_MSG_RETRY_INTERVAL() + " = " + settings.value(SETTING_SEND_MSG_RETRY_INTERVAL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_LONGPOLL_RETRY_INTERVAL() + " = " + settings.value(SETTING_LONGPOLL_RETRY_INTERVAL()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_LOCAL_PROXY_HOST() + " = " + settings.value(SETTING_LOCAL_PROXY_HOST()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_LOCAL_PROXY_PORT() + " = " + settings.value(SETTING_LOCAL_PROXY_PORT()).toString());
    LOG_DEBUG(logger, "SETTING: " + SETTING_PERSISTENCE_FILENAME() + " = " + settings.value(SETTING_PERSISTENCE_FILENAME()).toString());
}


} // namespace joynr

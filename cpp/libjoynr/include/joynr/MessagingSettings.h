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
#ifndef MESSAGINGSETTINGS_H
#define MESSAGINGSETTINGS_H

#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"
#include <QObject>
#include <QSettings>

namespace joynr {

class BounceProxyUrl;

class JOYNR_EXPORT MessagingSettings : public QObject {
    Q_OBJECT

public:
    explicit MessagingSettings(QSettings& settings, QObject* parent = 0);
    MessagingSettings(const MessagingSettings& other);

    ~MessagingSettings();

    static const QString& SETTING_BOUNCE_PROXY_URL();
    static const QString& SETTING_DISCOVERY_DIRECTORIES_DOMAIN();
    static const QString& SETTING_CHANNEL_URL_DIRECTORY_URL();
    static const QString& SETTING_CHANNEL_URL_DIRECTORY_CHANNELID();
    static const QString& SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID();
    static const QString& SETTING_CAPABILITIES_DIRECTORY_URL();
    static const QString& SETTING_CAPABILITIES_DIRECTORY_CHANNELID();
    static const QString& SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID();
    static const QString& SETTING_INDEX();
    static const QString& SETTING_CREATE_CHANNEL_RETRY_INTERVAL();
    static const QString& SETTING_DELETE_CHANNEL_RETRY_INTERVAL();
    static const QString& SETTING_SEND_MSG_RETRY_INTERVAL();
    static const QString& SETTING_LONGPOLL_RETRY_INTERVAL();

    static const QString& SETTING_LOCAL_PROXY_HOST();
    static const QString& SETTING_LOCAL_PROXY_PORT();

    static const QString& SETTING_HTTP_DEBUG();
    static const QString& SETTING_PERSISTENCE_FILENAME();
    static const QString& SETTING_LONGPOLL_TIMEOUT_MS();
    static const QString& SETTING_HTTP_CONNECT_TIMEOUT_MS();
    static const QString& SETTING_BOUNCEPROXY_TIMEOUT_MS();
    static const QString& SETTING_SEND_MESSAGE_MAX_TTL();

    static const QString& DEFAULT_MESSAGING_SETTINGS_FILENAME();
    static const QString& DEFAULT_PERSISTENCE_FILENAME();
    static qint64 DEFAULT_LONGPOLL_TIMEOUT_MS();
    static qint64 DEFAULT_HTTP_CONNECT_TIMEOUT_MS();
    static qint64 DEFAULT_BOUNCEPROXY_TIMEOUT_MS();
    static qint64 DEFAULT_SEND_MESSAGE_MAX_TTL();


    BounceProxyUrl getBounceProxyUrl() const;
    void setBounceProxyUrl(const BounceProxyUrl& bounceProxyUrl);

    QString getDiscoveryDirectoriesDomain() const;

    QString getChannelUrlDirectoryUrl() const;
    QString getChannelUrlDirectoryChannelId() const;
    QString getChannelUrlDirectoryParticipantId() const;

    QString getCapabilitiesDirectoryUrl() const;
    QString getCapabilitiesDirectoryChannelId() const;
    QString getCapabilitiesDirectoryParticipantId() const;

    qint64 getIndex() const;
    void setIndex(qint64 index);
    int getCreateChannelRetryInterval() const;
    void setCreateChannelRetryInterval(const int& retryInterval);
    int getDeleteChannelRetryInterval() const;
    void setDeleteChannelRetryInterval(const int& retryInterval);
    int getSendMsgRetryInterval() const;
    void setSendMsgRetryInterval(const int& retryInterval);
    int getLongPollRetryInterval() const;
    void setLongPollRetryInterval(const int& retryInterval);
    QString getLocalProxyHost() const;
    void setLocalProxyHost(const QString& localProxyHost);
    QString getLocalProxyPort() const;
    void setLocalProxyPort(const int& localProxyPort);
    void setHttpDebug(const bool& httpDebug);
    bool getHttpDebug() const;
    QString getMessagingPropertiesPersistenceFilename() const;
    void setMessagingPropertiesPersistenceFilename(const QString& persistenceFilename);
    qint64 getLongPollTimeout() const;
    void setLongPollTimeout(qint64 timeout_ms);
    qint64 getHttpConnectTimeout() const;
    void setHttpConnectTimeout(qint64 timeout_ms);
    qint64 getBounceProxyTimeout() const;
    void setBounceProxyTimeout(qint64 timeout_ms);
    qint64 getSendMsgMaxTtl() const;
    void setSendMsgMaxTtl(qint64 ttl_ms);

    bool contains(const QString& key) const;
    QVariant value(const QString& key) const;

signals:

public slots:
    void printSettings() const;

private:
    void operator=(const MessagingSettings& other);

    QSettings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};


} // namespace joynr
#endif // MESSAGINGSETTINGS_H

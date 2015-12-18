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

#include <string>

namespace joynr
{

class BounceProxyUrl;
class Settings;

class JOYNR_EXPORT MessagingSettings
{
public:
    explicit MessagingSettings(Settings& settings);
    MessagingSettings(const MessagingSettings& other);

    ~MessagingSettings();

    static const std::string& SETTING_BOUNCE_PROXY_URL();
    static const std::string& SETTING_DISCOVERY_DIRECTORIES_DOMAIN();
    static const std::string& SETTING_CHANNEL_URL_DIRECTORY_URL();
    static const std::string& SETTING_CHANNEL_URL_DIRECTORY_CHANNELID();
    static const std::string& SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_URL();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_CHANNELID();
    static const std::string& SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID();
    static const std::string& SETTING_INDEX();
    static const std::string& SETTING_CREATE_CHANNEL_RETRY_INTERVAL();
    static const std::string& SETTING_DELETE_CHANNEL_RETRY_INTERVAL();
    static const std::string& SETTING_SEND_MSG_RETRY_INTERVAL();
    static const std::string& SETTING_LONGPOLL_RETRY_INTERVAL();

    static const std::string& SETTING_LOCAL_PROXY_HOST();
    static const std::string& SETTING_LOCAL_PROXY_PORT();

    static const std::string& SETTING_CERTIFICATE_AUTHORITY();
    static const std::string& SETTING_CLIENT_CERTIFICATE();
    static const std::string& SETTING_CLIENT_CERTIFICATE_PASSWORD();

    static const std::string& SETTING_HTTP_DEBUG();
    static const std::string& SETTING_PERSISTENCE_FILENAME();
    static const std::string& SETTING_LONGPOLL_TIMEOUT_MS();
    static const std::string& SETTING_HTTP_CONNECT_TIMEOUT_MS();
    static const std::string& SETTING_BOUNCEPROXY_TIMEOUT_MS();
    /**
     * @brief SETTING_DISCOVERY_MESSAGES_TTL_MS Time-to-live of messages used
     * in communication between the local discovery service and the discovery
     * backend service.
     *
     * @return the TTL used for discovery messages send to the backend
     * discovery service.
     */
    static const std::string& SETTING_DISCOVERY_MESSAGES_TTL_MS();
    static const std::string& SETTING_SEND_MESSAGE_MAX_TTL();

    static const std::string& DEFAULT_MESSAGING_SETTINGS_FILENAME();
    static const std::string& DEFAULT_PERSISTENCE_FILENAME();
    static int64_t DEFAULT_LONGPOLL_TIMEOUT_MS();
    static int64_t DEFAULT_HTTP_CONNECT_TIMEOUT_MS();
    static int64_t DEFAULT_BOUNCEPROXY_TIMEOUT_MS();
    static int64_t DEFAULT_DISCOVERY_REQUEST_TIMEOUT_MS();
    static int64_t DEFAULT_SEND_MESSAGE_MAX_TTL();

    BounceProxyUrl getBounceProxyUrl() const;
    std::string getBounceProxyUrlString() const;
    void setBounceProxyUrl(const BounceProxyUrl& bounceProxyUrl);

    std::string getDiscoveryDirectoriesDomain() const;

    std::string getChannelUrlDirectoryUrl() const;
    std::string getChannelUrlDirectoryChannelId() const;
    std::string getChannelUrlDirectoryParticipantId() const;

    std::string getCapabilitiesDirectoryUrl() const;
    std::string getCapabilitiesDirectoryChannelId() const;
    std::string getCapabilitiesDirectoryParticipantId() const;

    int64_t getIndex() const;
    void setIndex(int64_t index);
    int getCreateChannelRetryInterval() const;
    void setCreateChannelRetryInterval(const int& retryInterval);
    int getDeleteChannelRetryInterval() const;
    void setDeleteChannelRetryInterval(const int& retryInterval);
    int getSendMsgRetryInterval() const;
    void setSendMsgRetryInterval(const int& retryInterval);
    int getLongPollRetryInterval() const;
    void setLongPollRetryInterval(const int& retryInterval);
    std::string getLocalProxyHost() const;
    void setLocalProxyHost(const std::string& localProxyHost);
    std::string getLocalProxyPort() const;
    void setLocalProxyPort(const int& localProxyPort);
    void setHttpDebug(const bool& httpDebug);
    bool getHttpDebug() const;
    std::string getCertificateAuthority() const;
    void setCertificateAuthority(const std::string& certificateAuthority);
    std::string getClientCertificate() const;
    void setClientCertificate(const std::string& clientCertificate);
    std::string getClientCertificatePassword() const;
    void setClientCertificatePassword(const std::string& clientCertificatePassword);
    std::string getMessagingPropertiesPersistenceFilename() const;
    void setMessagingPropertiesPersistenceFilename(const std::string& persistenceFilename);
    int64_t getLongPollTimeout() const;
    void setLongPollTimeout(int64_t timeout_ms);
    int64_t getHttpConnectTimeout() const;
    void setHttpConnectTimeout(int64_t timeout_ms);
    int64_t getBounceProxyTimeout() const;
    void setBounceProxyTimeout(int64_t timeout_ms);
    int64_t getDiscoveryMessagesTtl() const;
    void setDiscoveryMessagesTtl(int64_t ttl_ms);
    int64_t getSendMsgMaxTtl() const;
    void setSendMsgMaxTtl(int64_t ttl_ms);

    bool contains(const std::string& key) const;

    void printSettings() const;

private:
    void operator=(const MessagingSettings& other);

    Settings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};

} // namespace joynr
#endif // MESSAGINGSETTINGS_H

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
#ifndef BOUNCEPROXYURL_H
#define BOUNCEPROXYURL_H

#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"

#include <QObject>
#include <QUrl>
#include <string>

namespace joynr
{

class JOYNR_EXPORT BounceProxyUrl : public QObject
{
    Q_OBJECT
public:
    explicit BounceProxyUrl(const std::string& bounceProxyChannelsBaseUrl, QObject* parent = 0);

    BounceProxyUrl(const BounceProxyUrl& other);

    BounceProxyUrl& operator=(const BounceProxyUrl& bounceProxyUrl);
    bool operator==(const BounceProxyUrl& bounceProxyUrl) const;

    static const std::string& CREATE_CHANNEL_QUERY_ITEM();
    static const std::string& SEND_MESSAGE_PATH_APPENDIX();
    static const std::string& CHANNEL_PATH_SUFFIX();
    static const std::string& TIMECHECK_PATH_SUFFIX();
    static const std::string& URL_PATH_SEPARATOR();

    QUrl getCreateChannelUrl(const std::string& mcid) const;
    QUrl getReceiveUrl(const std::string& channelId) const;
    QUrl getSendUrl(const std::string& channelId) const;
    QUrl getBounceProxyBaseUrl() const;
    QUrl getDeleteChannelUrl(const std::string& mcid) const;
    QUrl getTimeCheckUrl() const;

private:
    std::string bounceProxyBaseUrl;
    QUrl bounceProxyChannelsBaseUrl;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // BOUNCEPROXYURL_H

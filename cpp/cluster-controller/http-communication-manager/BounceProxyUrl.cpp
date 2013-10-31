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
#include <QtCore/QUrlQuery>
#include "cluster-controller/http-communication-manager/BounceProxyUrl.h"

namespace joynr {

joynr_logging::Logger* BounceProxyUrl::logger = joynr_logging::Logging::getInstance()->getLogger("JOYNR", "BounceProxyUrl");

const QString& BounceProxyUrl::URL_PATH_SEPARATOR() {
    static const QString value("/");
    return value;
}

const QString& BounceProxyUrl::CREATE_CHANNEL_QUERY_ITEM() {
    static const QString value("ccid");
    return value;
}

const QString& BounceProxyUrl::SEND_MESSAGE_PATH_APPENDIX() {
    static const QString value("message");
    return value;
}

const QString& BounceProxyUrl::CHANNEL_PATH_SUFFIX() {
    static const QString value("channels");
    return value;
}

const QString& BounceProxyUrl::TIMECHECK_PATH_SUFFIX() {
    static const QString value("time");
    return value;
}

BounceProxyUrl::BounceProxyUrl(const QString& bounceProxyBaseUrl, QObject* parent) :
    QObject(parent),
    bounceProxyBaseUrl(bounceProxyBaseUrl),
    bounceProxyChannelsBaseUrl()
{
    if(!this->bounceProxyBaseUrl.endsWith(URL_PATH_SEPARATOR())){
        this->bounceProxyBaseUrl.append(URL_PATH_SEPARATOR());
    }
    QString channelsBaseUrl(this->bounceProxyBaseUrl);
    channelsBaseUrl.append(CHANNEL_PATH_SUFFIX());
    channelsBaseUrl.append(URL_PATH_SEPARATOR());
    this->bounceProxyChannelsBaseUrl = QUrl(channelsBaseUrl);
}

BounceProxyUrl::BounceProxyUrl(const BounceProxyUrl& other) :
        QObject(other.parent()),
        bounceProxyBaseUrl(other.bounceProxyBaseUrl),
        bounceProxyChannelsBaseUrl(other.bounceProxyChannelsBaseUrl)
{

}

BounceProxyUrl& BounceProxyUrl::operator=(const BounceProxyUrl& bounceProxyUrl) {
    bounceProxyChannelsBaseUrl = bounceProxyUrl.bounceProxyChannelsBaseUrl;
    return *this;
}

bool BounceProxyUrl::operator==(const BounceProxyUrl& bounceProxyUrl) const {
    return bounceProxyChannelsBaseUrl == bounceProxyUrl.getBounceProxyBaseUrl();
}


QUrl BounceProxyUrl::getCreateChannelUrl(const QString& mcid) const {
    QUrl createChannelUrl(bounceProxyChannelsBaseUrl);
    QUrlQuery query;
    query.addQueryItem(CREATE_CHANNEL_QUERY_ITEM(), mcid);
    createChannelUrl.setQuery(query);
    return createChannelUrl;
}

QUrl BounceProxyUrl::getSendUrl(const QString& channelId) const {
    QUrl sendUrl(bounceProxyChannelsBaseUrl);
    QString path = sendUrl.path();
    if(!path.endsWith(URL_PATH_SEPARATOR())) {
        path.append(URL_PATH_SEPARATOR());
    }
    path.append(channelId);
    path.append(URL_PATH_SEPARATOR());
    path.append(SEND_MESSAGE_PATH_APPENDIX());
    path.append(URL_PATH_SEPARATOR());
    sendUrl.setPath(path);
    return sendUrl;
}

QUrl BounceProxyUrl::getBounceProxyBaseUrl() const {
    QUrl sendUrl(bounceProxyChannelsBaseUrl);
    QString path = sendUrl.path();
    if(!path.endsWith(URL_PATH_SEPARATOR())) {
        path.append(URL_PATH_SEPARATOR());
    }
    sendUrl.setPath(path);
    return sendUrl;
}

QUrl BounceProxyUrl::getDeleteChannelUrl(const QString& mcid) const {
    QUrl sendUrl(bounceProxyChannelsBaseUrl);
    QString path = sendUrl.path();
    path.append(mcid);
    path.append(URL_PATH_SEPARATOR());
    sendUrl.setPath(path);
    return sendUrl;
}

QUrl BounceProxyUrl::getTimeCheckUrl() const
{
    QUrl timeCheckUrl(bounceProxyBaseUrl);
    QString path = timeCheckUrl.path();
    path.append(TIMECHECK_PATH_SUFFIX());
    path.append(URL_PATH_SEPARATOR());
    timeCheckUrl.setPath(path);
    return timeCheckUrl;
}

} // namespace joynr

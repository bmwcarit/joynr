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
#include "joynr/BounceProxyUrl.h"
#include <boost/algorithm/string/predicate.hpp>

namespace joynr
{

joynr_logging::Logger* BounceProxyUrl::logger =
        joynr_logging::Logging::getInstance()->getLogger("JOYNR", "BounceProxyUrl");

const std::string& BounceProxyUrl::URL_PATH_SEPARATOR()
{
    static const std::string value("/");
    return value;
}

const std::string& BounceProxyUrl::CREATE_CHANNEL_QUERY_ITEM()
{
    static const std::string value("ccid");
    return value;
}

const std::string& BounceProxyUrl::SEND_MESSAGE_PATH_APPENDIX()
{
    static const std::string value("message");
    return value;
}

const std::string& BounceProxyUrl::CHANNEL_PATH_SUFFIX()
{
    static const std::string value("channels");
    return value;
}

const std::string& BounceProxyUrl::TIMECHECK_PATH_SUFFIX()
{
    static const std::string value("time");
    return value;
}

BounceProxyUrl::BounceProxyUrl(const std::string& bounceProxyBaseUrl)
        : bounceProxyBaseUrl(bounceProxyBaseUrl), bounceProxyChannelsBaseUrl()
{
    using boost::algorithm::ends_with;
    if (!ends_with(this->bounceProxyBaseUrl, URL_PATH_SEPARATOR())) {
        this->bounceProxyBaseUrl.append(URL_PATH_SEPARATOR());
    }
    std::string channelsBaseUrl(this->bounceProxyBaseUrl);
    channelsBaseUrl.append(CHANNEL_PATH_SUFFIX());
    channelsBaseUrl.append(URL_PATH_SEPARATOR());
    this->bounceProxyChannelsBaseUrl = Url(channelsBaseUrl);
}

BounceProxyUrl::BounceProxyUrl(const BounceProxyUrl& other)
        : bounceProxyBaseUrl(other.bounceProxyBaseUrl),
          bounceProxyChannelsBaseUrl(other.bounceProxyChannelsBaseUrl)
{
}

BounceProxyUrl& BounceProxyUrl::operator=(const BounceProxyUrl& bounceProxyUrl)
{
    bounceProxyChannelsBaseUrl = bounceProxyUrl.bounceProxyChannelsBaseUrl;
    return *this;
}

bool BounceProxyUrl::operator==(const BounceProxyUrl& bounceProxyUrl) const
{
    return bounceProxyChannelsBaseUrl == bounceProxyUrl.getBounceProxyBaseUrl();
}

Url BounceProxyUrl::getCreateChannelUrl(const std::string& mcid) const
{
    Url createChannelUrl(bounceProxyChannelsBaseUrl);
    UrlQuery query;
    query.addQueryItem(CREATE_CHANNEL_QUERY_ITEM(), mcid);
    createChannelUrl.setQuery(query);

    return createChannelUrl;
}

Url BounceProxyUrl::getSendUrl(const std::string& channelId) const
{
    Url sendUrl(bounceProxyChannelsBaseUrl);
    std::string path = sendUrl.getPath();
    using boost::algorithm::ends_with;
    if (!ends_with(path, URL_PATH_SEPARATOR())) {
        path.append(URL_PATH_SEPARATOR());
    }
    path.append(channelId);
    path.append(URL_PATH_SEPARATOR());
    path.append(SEND_MESSAGE_PATH_APPENDIX());
    path.append(URL_PATH_SEPARATOR());
    sendUrl.setPath(path);
    return sendUrl;
}

Url BounceProxyUrl::getBounceProxyBaseUrl() const
{
    Url sendUrl(bounceProxyChannelsBaseUrl);
    std::string path = sendUrl.getPath();
    using boost::algorithm::ends_with;
    if (!ends_with(path, URL_PATH_SEPARATOR())) {
        path.append(URL_PATH_SEPARATOR());
    }
    sendUrl.setPath(path);
    return sendUrl;
}

Url BounceProxyUrl::getDeleteChannelUrl(const std::string& mcid) const
{
    Url sendUrl(bounceProxyChannelsBaseUrl);
    std::string path = sendUrl.getPath();
    path.append(mcid);
    path.append(URL_PATH_SEPARATOR());
    sendUrl.setPath(path);
    return sendUrl;
}

Url BounceProxyUrl::getTimeCheckUrl() const
{
    Url timeCheckUrl(bounceProxyBaseUrl);
    std::string path = timeCheckUrl.getPath();
    path.append(TIMECHECK_PATH_SUFFIX());
    path.append(URL_PATH_SEPARATOR());
    timeCheckUrl.setPath(path);
    return timeCheckUrl;
}

} // namespace joynr

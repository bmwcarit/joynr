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
#include "joynr/JoynrMessage.h"

#include <iostream>
#include <string>
#include <QUuid>

#include "joynr/Util.h"

namespace joynr
{

using namespace joynr_logging;

Logger* JoynrMessage::logger = Logging::getInstance()->getLogger("MSG", "JoynrMessage");

const std::string& JoynrMessage::HEADER_CONTENT_TYPE()
{
    static const std::string headerContentType("contentType");
    return headerContentType;
}

const std::string& JoynrMessage::HEADER_MESSAGE_ID()
{
    static const std::string headerMessageId("msgId");
    return headerMessageId;
}

const std::string& JoynrMessage::HEADER_CREATOR_USER_ID()
{
    static const std::string headerCreatorUserId("creator");
    return headerCreatorUserId;
}

const std::string& JoynrMessage::HEADER_TO()
{
    static const std::string headerTo("to");
    return headerTo;
}
const std::string& JoynrMessage::HEADER_FROM()
{
    static const std::string headerFrom("from");
    return headerFrom;
}
const std::string& JoynrMessage::HEADER_EXPIRY_DATE()
{
    static const std::string headerExpiryDate("expiryDate");
    return headerExpiryDate;
}
const std::string& JoynrMessage::HEADER_REPLY_CHANNEL_ID()
{
    static const std::string headerReplyChannelId("replyChannelId");
    return headerReplyChannelId;
}

const std::string JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY = "oneWay";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_REPLY = "reply";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST = "request";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST =
        "broadcastSubscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";

const std::string JoynrMessage::VALUE_CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const std::string JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON = "application/json";

JoynrMessage::JoynrMessage() : type(""), headerMap(), payload()
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage::JoynrMessage(const JoynrMessage& message)
        : QObject(), type(message.type), headerMap(message.headerMap), payload(message.payload)
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage& JoynrMessage::operator=(const JoynrMessage& message)
{
    type = message.type;
    headerMap = message.headerMap;
    payload = message.payload;
    generateAndSetMsgIdHeaderIfAbsent();
    return *this;
}

void JoynrMessage::generateAndSetMsgIdHeaderIfAbsent()
{
    if (!containsHeader(HEADER_MESSAGE_ID())) {
        std::string msgId = Util::createUuid().toStdString();
        setHeader(HEADER_MESSAGE_ID(), msgId);
    }
}

bool JoynrMessage::operator==(const JoynrMessage& message) const
{
    return type == message.getType() && payload == message.payload &&
           headerMap == message.headerMap;
}

std::string JoynrMessage::getType() const
{
    return type;
}

void JoynrMessage::setType(const std::string& type)
{
    this->type = type;
}

std::map<std::string, std::string> JoynrMessage::getHeaderMap() const
{
    return headerMap;
}

bool JoynrMessage::containsHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator pos = headerMap.find(key);
    if (pos == headerMap.end()) {
        return false;
    }

    return true;
}

void JoynrMessage::setHeaderMap(const std::map<std::string, std::string>& newHeaders)
{
    std::map<std::string, std::string>::const_iterator i = newHeaders.begin();
    while (i != newHeaders.end()) {
        if (!containsHeader(i->first)) {
            headerMap.insert(std::pair<std::string, std::string>(i->first, i->second));
            LOG_DEBUG(logger,
                      QString("insert header: %1=%2").arg(QString::fromStdString(i->first)).arg(
                              QString::fromStdString(i->second)));
        }
        i++;
    }
}

std::string JoynrMessage::getHeader(const std::string& key) const
{
    // to avoid adding default-constructed value to the map, I use find instead of operator[]
    std::map<std::string, std::string>::const_iterator pos = headerMap.find(key);
    if (pos == headerMap.end()) {
        return std::string();
    }
    std::string value = pos->second;
    return value;
}

void JoynrMessage::setHeader(const std::string& key, const std::string& value)
{
    headerMap[key] = value;
}

QByteArray JoynrMessage::getPayload() const
{
    return payload;
}

void JoynrMessage::setPayload(const QByteArray& payload)
{
    this->payload = payload;
}

bool JoynrMessage::containsHeaderContentType() const
{
    return containsHeader(HEADER_CONTENT_TYPE());
}

std::string JoynrMessage::getHeaderContentType() const
{
    return getHeader(HEADER_CONTENT_TYPE());
}

void JoynrMessage::setHeaderContentType(const std::string& contentType)
{
    setHeader(HEADER_CONTENT_TYPE(), contentType);
}

bool JoynrMessage::containsHeaderMessageId() const
{
    return containsHeader(HEADER_MESSAGE_ID());
}

std::string JoynrMessage::getHeaderMessageId() const
{
    return getHeader(HEADER_MESSAGE_ID());
}

void JoynrMessage::setHeaderMessageId(const std::string& msgId)
{
    setHeader(HEADER_MESSAGE_ID(), msgId);
}

bool JoynrMessage::containsHeaderCreatorUserId() const
{
    return containsHeader(HEADER_CREATOR_USER_ID());
}

std::string JoynrMessage::getHeaderCreatorUserId() const
{
    return getHeader(HEADER_CREATOR_USER_ID());
}

void JoynrMessage::setHeaderCreatorUserId(const std::string& creatorUserId)
{
    LOG_TRACE(logger,
              QString("########## header creater user id: %1")
                      .arg(QString::fromStdString(HEADER_CREATOR_USER_ID())));
    setHeader(HEADER_CREATOR_USER_ID(), creatorUserId);
}

bool JoynrMessage::containsHeaderTo() const
{
    return containsHeader(HEADER_TO());
}

std::string JoynrMessage::getHeaderTo() const
{
    return getHeader(HEADER_TO());
}

void JoynrMessage::setHeaderTo(const std::string& to)
{
    setHeader(HEADER_TO(), to);
}

bool JoynrMessage::containsHeaderFrom() const
{
    return containsHeader(HEADER_FROM());
}

std::string JoynrMessage::getHeaderFrom() const
{
    return getHeader(HEADER_FROM());
}

void JoynrMessage::setHeaderFrom(const std::string& from)
{
    setHeader(HEADER_FROM(), from);
}

bool JoynrMessage::containsHeaderExpiryDate() const
{
    return containsHeader(HEADER_EXPIRY_DATE());
}

JoynrTimePoint JoynrMessage::getHeaderExpiryDate() const
{
    std::string expiryDateString = getHeader(HEADER_EXPIRY_DATE());
    JoynrTimePoint expiryDate{std::chrono::milliseconds(std::stoll(expiryDateString))};
    return expiryDate;
}

void JoynrMessage::setHeaderExpiryDate(const JoynrTimePoint& expiryDate)
{
    setHeader(HEADER_EXPIRY_DATE(), std::to_string(expiryDate.time_since_epoch().count()));
}

bool JoynrMessage::containsHeaderReplyChannelId() const
{
    return containsHeader(HEADER_REPLY_CHANNEL_ID());
}

std::string JoynrMessage::getHeaderReplyChannelId() const
{
    return getHeader(HEADER_REPLY_CHANNEL_ID());
}

void JoynrMessage::setHeaderReplyChannelId(const std::string& replyChannelId)
{
    setHeader(HEADER_REPLY_CHANNEL_ID(), replyChannelId);
}

} // namespace joynr

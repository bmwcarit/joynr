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
#include <QString>
#include <QUuid>

#include "joynr/Util.h"

namespace joynr
{

using namespace joynr_logging;

Logger* JoynrMessage::logger = Logging::getInstance()->getLogger("MSG", "JoynrMessage");

const QString& JoynrMessage::HEADER_CONTENT_TYPE()
{
    static const QString headerContentType("contentType");
    return headerContentType;
}

const QString& JoynrMessage::HEADER_MESSAGE_ID()
{
    static const QString headerMessageId("msgId");
    return headerMessageId;
}

const QString& JoynrMessage::HEADER_CREATOR_USER_ID()
{
    static const QString headerCreatorUserId("creator");
    return headerCreatorUserId;
}

const QString& JoynrMessage::HEADER_TO()
{
    static const QString headerTo("to");
    return headerTo;
}
const QString& JoynrMessage::HEADER_FROM()
{
    static const QString headerFrom("from");
    return headerFrom;
}
const QString& JoynrMessage::HEADER_EXPIRY_DATE()
{
    static const QString headerExpiryDate("expiryDate");
    return headerExpiryDate;
}
const QString& JoynrMessage::HEADER_REPLY_CHANNEL_ID()
{
    static const QString headerReplyChannelId("replyChannelId");
    return headerReplyChannelId;
}

const QString JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY = "oneWay";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_REPLY = "reply";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST = "request";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST =
        "broadcastSubscriptionRequest";
const QString JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";

const QString JoynrMessage::VALUE_CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const QString JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON = "application/json";

JoynrMessage::JoynrMessage() : type(""), header(QVariantMap()), payload()
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage::JoynrMessage(const JoynrMessage& message)
        : QObject(), type(message.type), header(message.header), payload(message.payload)
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage& JoynrMessage::operator=(const JoynrMessage& message)
{
    type = message.type;
    header = message.header;
    payload = message.payload;
    generateAndSetMsgIdHeaderIfAbsent();
    return *this;
}

void JoynrMessage::generateAndSetMsgIdHeaderIfAbsent()
{
    if (!containsHeader(HEADER_MESSAGE_ID())) {
        QString msgId = Util::createUuid();
        setHeader<QString>(HEADER_MESSAGE_ID(), msgId);
    }
}

bool JoynrMessage::operator==(const JoynrMessage& message) const
{
    return type == message.getType() && payload == message.payload && header == message.header;
}

QString JoynrMessage::getType() const
{
    return type;
}

void JoynrMessage::setType(const QString& type)
{
    this->type = type;
}

QVariant JoynrMessage::getHeader() const
{
    return header;
}

bool JoynrMessage::containsHeader(const QString& key) const
{
    return header.toMap().contains(key);
}

/**
 * @brief JoynrMessage::setHeader Adds header entries to the already existing ones.
 * If a header entry was already set, its value is replaced with the new one.
 * @param header the header entries to add
 */
void JoynrMessage::setHeader(const QVariant& header)
{
    QVariantMap headerMap = header.toMap();
    QMapIterator<QString, QVariant> i(this->header.toMap());
    while (i.hasNext()) {
        i.next();
        if (!headerMap.contains(i.key())) {
            headerMap.insert(i.key(), i.value());
            LOG_DEBUG(logger,
                      QString("insert header: %1=%2").arg(i.key()).arg(i.value().value<QString>()));
        }
    }
    this->header = headerMap;
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

QString JoynrMessage::getHeaderContentType() const
{
    return getHeader<QString>(HEADER_CONTENT_TYPE());
}

void JoynrMessage::setHeaderContentType(const QString& contentType)
{
    setHeader<QString>(HEADER_CONTENT_TYPE(), contentType);
}

bool JoynrMessage::containsHeaderMessageId() const
{
    return containsHeader(HEADER_MESSAGE_ID());
}

QString JoynrMessage::getHeaderMessageId() const
{
    return getHeader<QString>(HEADER_MESSAGE_ID());
}

void JoynrMessage::setHeaderMessageId(const QString& msgId)
{
    setHeader<QString>(HEADER_MESSAGE_ID(), msgId);
}

bool JoynrMessage::containsHeaderCreatorUserId() const
{
    return containsHeader(HEADER_CREATOR_USER_ID());
}

QString JoynrMessage::getHeaderCreatorUserId() const
{
    return getHeader<QString>(HEADER_CREATOR_USER_ID());
}

void JoynrMessage::setHeaderCreatorUserId(const QString& creatorUserId)
{
    setHeader<QString>(HEADER_CREATOR_USER_ID(), creatorUserId);
}

bool JoynrMessage::containsHeaderTo() const
{
    return containsHeader(HEADER_TO());
}

QString JoynrMessage::getHeaderTo() const
{
    return getHeader<QString>(HEADER_TO());
}

void JoynrMessage::setHeaderTo(const QString& to)
{
    setHeader<QString>(HEADER_TO(), to);
}

bool JoynrMessage::containsHeaderFrom() const
{
    return containsHeader(HEADER_FROM());
}

QString JoynrMessage::getHeaderFrom() const
{
    return getHeader<QString>(HEADER_FROM());
}

void JoynrMessage::setHeaderFrom(const QString& from)
{
    setHeader<QString>(HEADER_FROM(), from);
}

bool JoynrMessage::containsHeaderExpiryDate() const
{
    return containsHeader(HEADER_EXPIRY_DATE());
}

JoynrTimePoint JoynrMessage::getHeaderExpiryDate() const
{
    JoynrTimePoint expiryDate{std::chrono::milliseconds(getHeader<qint64>(HEADER_EXPIRY_DATE()))};
    return expiryDate;
}

void JoynrMessage::setHeaderExpiryDate(const JoynrTimePoint& expiryDate)
{
    setHeader<qint64>(HEADER_EXPIRY_DATE(), expiryDate.time_since_epoch().count());
}

bool JoynrMessage::containsHeaderReplyChannelId() const
{
    return containsHeader(HEADER_REPLY_CHANNEL_ID());
}

QString JoynrMessage::getHeaderReplyChannelId() const
{
    return getHeader<QString>(HEADER_REPLY_CHANNEL_ID());
}

void JoynrMessage::setHeaderReplyChannelId(const QString& replyChannelId)
{
    setHeader<QString>(HEADER_REPLY_CHANNEL_ID(), replyChannelId);
}

} // namespace joynr

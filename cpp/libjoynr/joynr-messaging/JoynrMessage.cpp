/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include <sstream>
#include <utility>
#include <memory>
#include <chrono>

#include "joynr/Util.h"

namespace joynr
{
INIT_LOGGER(JoynrMessage);

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
const std::string& JoynrMessage::HEADER_REPLY_ADDRESS()
{
    static const std::string headerReplyAddress("replyChannelId");
    return headerReplyAddress;
}
const std::string& JoynrMessage::HEADER_EFFORT()
{
    static const std::string headerEffort("effort");
    return headerEffort;
}
const std::string& JoynrMessage::CUSTOM_HEADER_PREFIX()
{
    static const std::string customHeaderPrefix("custom-");
    return customHeaderPrefix;
}

const std::string JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID = "z4";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY = "oneWay";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_REPLY = "reply";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST = "request";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST = "multicast";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST =
        "multicastSubscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST =
        "broadcastSubscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";

const std::string JoynrMessage::VALUE_CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const std::string JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON = "application/json";

JoynrMessage::JoynrMessage()
        : type(""), header(), payload(), receivedFromGlobal(false), localMessage(false)
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage::JoynrMessage(const JoynrMessage& message)
        : type(message.type),
          header(message.header),
          payload(message.payload),
          receivedFromGlobal(false),
          localMessage(false)
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage& JoynrMessage::operator=(const JoynrMessage& message)
{
    type = message.type;
    header = message.header;
    payload = message.payload;
    receivedFromGlobal = message.receivedFromGlobal;
    localMessage = message.localMessage;
    generateAndSetMsgIdHeaderIfAbsent();
    return *this;
}

JoynrMessage::JoynrMessage(JoynrMessage&& message)
        : type(std::move(message.type)),
          header(std::move(message.header)),
          payload(std::move(message.payload)),
          receivedFromGlobal(std::move(message.receivedFromGlobal)),
          localMessage(std::move(message.localMessage))
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage& JoynrMessage::operator=(JoynrMessage&& message)
{
    type = std::move(message.type);
    header = std::move(message.header);
    payload = std::move(message.payload);
    receivedFromGlobal = std::move(message.receivedFromGlobal);
    localMessage = std::move(message.localMessage);
    generateAndSetMsgIdHeaderIfAbsent();
    return *this;
}

void JoynrMessage::generateAndSetMsgIdHeaderIfAbsent()
{
    if (!containsHeader(HEADER_MESSAGE_ID())) {
        std::string msgId = util::createUuid();
        setHeaderForKey(HEADER_MESSAGE_ID(), msgId);
    }
}

bool JoynrMessage::operator==(const JoynrMessage& message) const
{
    // Since receivedFromGlobal is a transient and auxiliary attribute which has nothing to do with
    // the identity of a JoynrMessage it is omitted from the comparison.
    return type == message.getType() && payload == message.payload && header == message.header;
}

std::string JoynrMessage::getType() const
{
    return type;
}

void JoynrMessage::setType(const std::string& type)
{
    this->type = type;
}

std::map<std::string, std::string> JoynrMessage::getHeader() const
{
    return header;
}

bool JoynrMessage::containsHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator pos = header.find(key);
    if (pos == header.end()) {
        return false;
    }

    return true;
}

void JoynrMessage::setHeader(const std::map<std::string, std::string>& newHeaders)
{
    std::map<std::string, std::string>::const_iterator i = newHeaders.begin();
    while (i != newHeaders.end()) {
        if (!containsHeader(i->first)) {
            header.insert(std::pair<std::string, std::string>(i->first, i->second));
            JOYNR_LOG_TRACE(logger, "insert header: {} = {}", i->second, i->first);
        } else {
            header[i->first] = i->second;
        }
        i++;
    }
}

std::string JoynrMessage::getHeaderForKey(const std::string& key) const
{
    // to avoid adding default-constructed value to the map, I use find instead of operator[]
    std::map<std::string, std::string>::const_iterator pos = header.find(key);
    if (pos == header.end()) {
        return std::string();
    }
    std::string value = pos->second;
    return value;
}

void JoynrMessage::setHeaderForKey(const std::string& key, const std::string& value)
{
    header[key] = value;
}

bool JoynrMessage::containsCustomHeader(const std::string& key) const
{
    return containsHeader(CUSTOM_HEADER_PREFIX() + key);
}

std::string JoynrMessage::getCustomHeader(const std::string& key) const
{
    return getHeaderForKey(CUSTOM_HEADER_PREFIX() + key);
}

void JoynrMessage::setCustomHeader(const std::string& key, const std::string& value)
{
    setHeaderForKey(CUSTOM_HEADER_PREFIX() + key, value);
}

std::string JoynrMessage::getPayload() const
{
    return payload;
}

void JoynrMessage::setPayload(const std::string& payload)
{
    this->payload = payload;
}

void JoynrMessage::setPayload(std::string&& payload)
{
    this->payload = std::move(payload);
}

bool JoynrMessage::containsHeaderContentType() const
{
    return containsHeader(HEADER_CONTENT_TYPE());
}

std::string JoynrMessage::getHeaderContentType() const
{
    return getHeaderForKey(HEADER_CONTENT_TYPE());
}

void JoynrMessage::setHeaderContentType(const std::string& contentType)
{
    setHeaderForKey(HEADER_CONTENT_TYPE(), contentType);
}

bool JoynrMessage::containsHeaderMessageId() const
{
    return containsHeader(HEADER_MESSAGE_ID());
}

std::string JoynrMessage::getHeaderMessageId() const
{
    return getHeaderForKey(HEADER_MESSAGE_ID());
}

void JoynrMessage::setHeaderMessageId(const std::string& msgId)
{
    setHeaderForKey(HEADER_MESSAGE_ID(), msgId);
}

bool JoynrMessage::containsHeaderCreatorUserId() const
{
    return containsHeader(HEADER_CREATOR_USER_ID());
}

std::string JoynrMessage::getHeaderCreatorUserId() const
{
    return getHeaderForKey(HEADER_CREATOR_USER_ID());
}

void JoynrMessage::setHeaderCreatorUserId(const std::string& creatorUserId)
{
    JOYNR_LOG_TRACE(logger, "########## header creator user id: {}", HEADER_CREATOR_USER_ID());
    setHeaderForKey(HEADER_CREATOR_USER_ID(), creatorUserId);
}

bool JoynrMessage::containsHeaderTo() const
{
    return containsHeader(HEADER_TO());
}

std::string JoynrMessage::getHeaderTo() const
{
    return getHeaderForKey(HEADER_TO());
}

void JoynrMessage::setHeaderTo(const std::string& to)
{
    setHeaderForKey(HEADER_TO(), to);
}

bool JoynrMessage::containsHeaderFrom() const
{
    return containsHeader(HEADER_FROM());
}

std::string JoynrMessage::getHeaderFrom() const
{
    return getHeaderForKey(HEADER_FROM());
}

void JoynrMessage::setHeaderFrom(const std::string& from)
{
    setHeaderForKey(HEADER_FROM(), from);
}

bool JoynrMessage::containsHeaderExpiryDate() const
{
    return containsHeader(HEADER_EXPIRY_DATE());
}

JoynrTimePoint JoynrMessage::getHeaderExpiryDate() const
{
    std::string expiryDateString = getHeaderForKey(HEADER_EXPIRY_DATE());
    JoynrTimePoint expiryDate(std::chrono::milliseconds(std::stoll(expiryDateString)));
    return expiryDate;
}

void JoynrMessage::setHeaderExpiryDate(const JoynrTimePoint& expiryDate)
{
    setHeaderForKey(HEADER_EXPIRY_DATE(), std::to_string(expiryDate.time_since_epoch().count()));
}

bool JoynrMessage::containsHeaderReplyAddress() const
{
    return containsHeader(HEADER_REPLY_ADDRESS());
}

std::string JoynrMessage::getHeaderReplyAddress() const
{
    return getHeaderForKey(HEADER_REPLY_ADDRESS());
}

void JoynrMessage::setHeaderReplyAddress(const std::string& replyAddress)
{
    setHeaderForKey(HEADER_REPLY_ADDRESS(), replyAddress);
}

bool JoynrMessage::containsHeaderEffort() const
{
    return containsHeader(HEADER_EFFORT());
}

std::string JoynrMessage::getHeaderEffort() const
{
    return getHeaderForKey(HEADER_EFFORT());
}

void JoynrMessage::setHeaderEffort(const std::string& effort)
{
    setHeaderForKey(HEADER_EFFORT(), effort);
}

bool JoynrMessage::isReceivedFromGlobal() const
{
    return receivedFromGlobal;
}

void JoynrMessage::setReceivedFromGlobal(bool receivedFromGlobal)
{
    this->receivedFromGlobal = receivedFromGlobal;
}

bool JoynrMessage::isLocalMessage() const
{
    return localMessage;
}

void JoynrMessage::setLocalMessage(bool localMessage)
{
    this->localMessage = localMessage;
}

std::string JoynrMessage::toLogMessage() const
{
    std::stringstream ss;
    ss << "type=" << type;
    ss << ", header={";
    for (auto it = header.cbegin(); it != header.cend(); ++it) {
        if (it != header.cbegin()) {
            ss << ", ";
        }
        ss << it->first << "=" << it->second;
    }
    ss << "}";
    ss << ", receivedFromGlobal=" << receivedFromGlobal;
    ss << ", localMessage=" << localMessage;
    return ss.str();
}

} // namespace joynr

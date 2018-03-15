/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/MutableMessage.h"

#include <smrf/MessageSerializer.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"

namespace joynr
{

MutableMessage::MutableMessage()
        : sender(),
          recipient(),
          keyChain(nullptr),
          expiryDate(),
          type(),
          id(util::createUuid()),
          replyTo(),
          effort(),
          customHeaders(),
          payload(),
          ttlAbsolute(true),
          localMessage(false),
          encrypt(false),
          compress(false)
{
}

std::unique_ptr<ImmutableMessage> MutableMessage::getImmutableMessage() const
{
    smrf::MessageSerializer messageSerializer;

    // propagate flags
    messageSerializer.setCompressed(compress);

    // explicit headers
    messageSerializer.setSender(sender);
    messageSerializer.setRecipient(recipient);
    messageSerializer.setTtlMs(expiryDate.toMilliseconds());

    // key-value pair headers
    std::unordered_map<std::string, std::string> keyValuePairHeaders;
    keyValuePairHeaders.insert({Message::HEADER_TYPE(), type});
    keyValuePairHeaders.insert({Message::HEADER_ID(), id});

    if (replyTo) {
        keyValuePairHeaders.insert({Message::HEADER_REPLY_TO(), *replyTo});
    }
    if (effort) {
        keyValuePairHeaders.insert({Message::HEADER_EFFORT(), *effort});
    }
    keyValuePairHeaders.insert(customHeaders.cbegin(), customHeaders.cend());
    messageSerializer.setHeaders(keyValuePairHeaders);

    smrf::ByteArrayView payloadView(
            reinterpret_cast<smrf::Byte*>(const_cast<char*>(payload.data())), payload.size());
    messageSerializer.setBody(payloadView);

    if (keyChain) {
        std::string ownerIdStr = keyChain->getOwnerId();
        smrf::ByteVector ownerIdSignature(ownerIdStr.begin(), ownerIdStr.end());
        auto ownersigningCallback =
                [ownerIdSignature](const smrf::ByteArrayView&) { return ownerIdSignature; };
        messageSerializer.setCustomSigningCallback(ownersigningCallback);
    }

    const bool verifyInput = false;
    return std::make_unique<ImmutableMessage>(messageSerializer.serialize(), verifyInput);
}

const std::string& MutableMessage::getRecipient() const
{
    return recipient;
}

const std::unordered_map<std::string, std::string>& MutableMessage::getCustomHeaders() const
{
    return customHeaders;
}

boost::optional<std::string> MutableMessage::getCustomHeader(const std::string& key) const
{
    const std::string lookupKey = Message::CUSTOM_HEADER_PREFIX() + key;
    boost::optional<std::string> value;
    auto it = customHeaders.find(lookupKey);
    if (it != customHeaders.cend()) {
        value = it->second;
    }
    return value;
}

void MutableMessage::setType(std::string&& type)
{
    this->type = std::move(type);
}

const std::string& MutableMessage::getType() const
{
    return type;
}

const std::string& MutableMessage::getId() const
{
    return id;
}

void MutableMessage::setReplyTo(const std::string& replyTo)
{
    this->replyTo = replyTo;
}

const boost::optional<std::string>& MutableMessage::getReplyTo() const
{
    return replyTo;
}

void MutableMessage::setCustomHeader(const std::string& key, const std::string& value)
{
    customHeaders.insert({Message::CUSTOM_HEADER_PREFIX() + key, value});
}

void MutableMessage::setCustomHeader(std::string&& key, std::string&& value)
{
    customHeaders.insert({Message::CUSTOM_HEADER_PREFIX() + std::move(key), std::move(value)});
}

void MutableMessage::setPrefixedCustomHeaders(
        const std::unordered_map<std::string, std::string>& prefixedCustomHeaders)
{
    customHeaders.insert(prefixedCustomHeaders.cbegin(), prefixedCustomHeaders.cend());
}

void MutableMessage::setPrefixedCustomHeaders(
        std::unordered_map<std::string, std::string>&& prefixedCustomHeaders)
{
    customHeaders.insert(std::make_move_iterator(prefixedCustomHeaders.begin()),
                         std::make_move_iterator(prefixedCustomHeaders.end()));
}

void MutableMessage::setEffort(const std::string& effort)
{
    this->effort = effort;
}

const boost::optional<std::string>& MutableMessage::getEffort() const
{
    return effort;
}

void MutableMessage::setPayload(const std::string& payload)
{
    this->payload = payload;
}

void MutableMessage::setPayload(std::string&& payload)
{
    this->payload = std::move(payload);
}

const std::string& MutableMessage::getPayload() const
{
    return payload;
}

bool MutableMessage::isLocalMessage() const
{
    return localMessage;
}

void MutableMessage::setLocalMessage(bool localMessage)
{
    this->localMessage = localMessage;
}

void MutableMessage::setEncrypt(bool encrypt)
{
    this->encrypt = encrypt;
}

bool MutableMessage::getEncrypt() const
{
    return encrypt;
}

void MutableMessage::setCompress(bool compress)
{
    this->compress = compress;
}

bool MutableMessage::getCompress() const
{
    return compress;
}

void MutableMessage::setEffort(std::string&& effort)
{
    this->effort = std::move(effort);
}

std::string MutableMessage::toLogMessage() const
{
    return serializer::serializeToJson(*this);
}

void MutableMessage::setType(const std::string& type)
{
    this->type = type;
}

TimePoint MutableMessage::getExpiryDate() const
{
    return expiryDate;
}

void MutableMessage::setExpiryDate(const TimePoint& expiryDate)
{
    this->expiryDate = expiryDate;
}

void MutableMessage::setRecipient(std::string&& recipient)
{
    this->recipient = std::move(recipient);
}

const std::string& MutableMessage::getSender() const
{
    return sender;
}

void MutableMessage::setRecipient(const std::string& recipient)
{
    this->recipient = recipient;
}

void MutableMessage::setSender(std::string&& sender)
{
    this->sender = std::move(sender);
}

void MutableMessage::setSender(const std::string& sender)
{
    this->sender = sender;
}

void MutableMessage::setKeychain(std::shared_ptr<IKeychain> keyChain)
{
    this->keyChain = std::move(keyChain);
}

void MutableMessage::setReplyTo(std::string&& replyTo)
{
    this->replyTo = std::move(replyTo);
}

} // namespace joynr

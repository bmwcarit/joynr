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
        : _sender(),
          _recipient(),
          _keyChain(nullptr),
          _expiryDate(),
          _type(),
          _id(util::createUuid()),
          _replyTo(),
          _effort(),
          _customHeaders(),
          _payload(),
          _ttlAbsolute(true),
          _localMessage(false),
          _encrypt(false),
          _compress(false)
{
}

std::unique_ptr<ImmutableMessage> MutableMessage::getImmutableMessage() const
{
    smrf::MessageSerializer messageSerializer;

    // propagate flags
    messageSerializer.setCompressed(_compress);

    // explicit headers
    messageSerializer.setSender(_sender);
    messageSerializer.setRecipient(_recipient);
    messageSerializer.setTtlMs(_expiryDate.toMilliseconds());

    // key-value pair headers
    std::unordered_map<std::string, std::string> keyValuePairHeaders;
    keyValuePairHeaders.insert({Message::HEADER_TYPE(), _type});
    keyValuePairHeaders.insert({Message::HEADER_ID(), _id});

    if (_replyTo) {
        keyValuePairHeaders.insert({Message::HEADER_REPLY_TO(), *_replyTo});
    }
    if (_effort) {
        keyValuePairHeaders.insert({Message::HEADER_EFFORT(), *_effort});
    }
    keyValuePairHeaders.insert(_customHeaders.cbegin(), _customHeaders.cend());
    messageSerializer.setHeaders(keyValuePairHeaders);

    smrf::ByteArrayView payloadView(
            reinterpret_cast<smrf::Byte*>(const_cast<char*>(_payload.data())), _payload.size());
    messageSerializer.setBody(payloadView);

    if (_keyChain) {
        std::string ownerIdStr = _keyChain->getOwnerId();
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
    return _recipient;
}

const std::unordered_map<std::string, std::string>& MutableMessage::getCustomHeaders() const
{
    return _customHeaders;
}

boost::optional<std::string> MutableMessage::getCustomHeader(const std::string& key) const
{
    const std::string lookupKey = Message::CUSTOM_HEADER_PREFIX() + key;
    boost::optional<std::string> value;
    auto it = _customHeaders.find(lookupKey);
    if (it != _customHeaders.cend()) {
        value = it->second;
    }
    return value;
}

void MutableMessage::setType(std::string&& type)
{
    this->_type = std::move(type);
}

const std::string& MutableMessage::getType() const
{
    return _type;
}

const std::string& MutableMessage::getId() const
{
    return _id;
}

void MutableMessage::setReplyTo(const std::string& replyTo)
{
    this->_replyTo = replyTo;
}

const boost::optional<std::string>& MutableMessage::getReplyTo() const
{
    return _replyTo;
}

void MutableMessage::setCustomHeader(const std::string& key, const std::string& value)
{
    _customHeaders.insert({Message::CUSTOM_HEADER_PREFIX() + key, value});
}

void MutableMessage::setCustomHeader(std::string&& key, std::string&& value)
{
    _customHeaders.insert({Message::CUSTOM_HEADER_PREFIX() + std::move(key), std::move(value)});
}

void MutableMessage::setPrefixedCustomHeaders(
        const std::unordered_map<std::string, std::string>& prefixedCustomHeaders)
{
    _customHeaders.insert(prefixedCustomHeaders.cbegin(), prefixedCustomHeaders.cend());
}

void MutableMessage::setPrefixedCustomHeaders(
        std::unordered_map<std::string, std::string>&& prefixedCustomHeaders)
{
    _customHeaders.insert(std::make_move_iterator(prefixedCustomHeaders.begin()),
                          std::make_move_iterator(prefixedCustomHeaders.end()));
}

void MutableMessage::setEffort(const std::string& effort)
{
    this->_effort = effort;
}

const boost::optional<std::string>& MutableMessage::getEffort() const
{
    return _effort;
}

void MutableMessage::setPayload(const std::string& payload)
{
    this->_payload = payload;
}

void MutableMessage::setPayload(std::string&& payload)
{
    this->_payload = std::move(payload);
}

const std::string& MutableMessage::getPayload() const
{
    return _payload;
}

bool MutableMessage::isLocalMessage() const
{
    return _localMessage;
}

void MutableMessage::setLocalMessage(bool localMessage)
{
    this->_localMessage = localMessage;
}

void MutableMessage::setEncrypt(bool encrypt)
{
    this->_encrypt = encrypt;
}

bool MutableMessage::getEncrypt() const
{
    return _encrypt;
}

void MutableMessage::setCompress(bool compress)
{
    this->_compress = compress;
}

bool MutableMessage::getCompress() const
{
    return _compress;
}

void MutableMessage::setEffort(std::string&& effort)
{
    this->_effort = std::move(effort);
}

std::string MutableMessage::toLogMessage() const
{
    return serializer::serializeToJson(*this);
}

void MutableMessage::setType(const std::string& type)
{
    this->_type = type;
}

TimePoint MutableMessage::getExpiryDate() const
{
    return _expiryDate;
}

void MutableMessage::setExpiryDate(const TimePoint& expiryDate)
{
    this->_expiryDate = expiryDate;
}

void MutableMessage::setRecipient(std::string&& recipient)
{
    this->_recipient = std::move(recipient);
}

const std::string& MutableMessage::getSender() const
{
    return _sender;
}

void MutableMessage::setRecipient(const std::string& recipient)
{
    this->_recipient = recipient;
}

void MutableMessage::setSender(std::string&& sender)
{
    this->_sender = std::move(sender);
}

void MutableMessage::setSender(const std::string& sender)
{
    this->_sender = sender;
}

void MutableMessage::setKeychain(std::shared_ptr<IKeychain> keyChain)
{
    this->_keyChain = std::move(keyChain);
}

void MutableMessage::setReplyTo(std::string&& replyTo)
{
    this->_replyTo = std::move(replyTo);
}

} // namespace joynr

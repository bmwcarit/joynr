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

#include "joynr/ImmutableMessage.h"

#include <cassert>
#include <stdexcept>
#include <utility>

#include "boost/algorithm/string.hpp"

#include "joynr/Message.h"

namespace joynr
{

ImmutableMessage::ImmutableMessage(smrf::ByteVector&& serializedMessage, bool verifyInput)
        : _serializedMessage(std::move(serializedMessage)),
          _messageDeserializer(smrf::ByteArrayView(this->_serializedMessage), verifyInput),
          headers(),
          _bodyView(),
          _decompressedBody(),
          receivedFromGlobal(false),
          _accessControlChecked(false),
          creator(),
          _requiredHeaders()
{
    init();
}

ImmutableMessage::ImmutableMessage(const smrf::ByteVector& serializedMessage, bool verifyInput)
        : _serializedMessage(serializedMessage),
          _messageDeserializer(smrf::ByteArrayView(this->_serializedMessage), verifyInput),
          headers(),
          _bodyView(),
          _decompressedBody(),
          receivedFromGlobal(false),
          _accessControlChecked(false),
          creator(),
          _requiredHeaders()
{
    init();
}

std::string ImmutableMessage::getSender() const
{
    return _messageDeserializer.getSender();
}

std::string ImmutableMessage::getRecipient() const
{
    return _messageDeserializer.getRecipient();
}

bool ImmutableMessage::isTtlAbsolute() const
{
    return _messageDeserializer.isTtlAbsolute();
}

const std::unordered_map<std::string, std::string>& ImmutableMessage::getHeaders() const
{
    return headers;
}

std::unordered_map<std::string, std::string> ImmutableMessage::getCustomHeaders() const
{
    if (headers.size() <= RequiredHeaders::NUM_REQUIRED_HEADERS) {
        return std::unordered_map<std::string, std::string>();
    }

    static std::size_t CUSTOM_HEADER_PREFIX_LENGTH = Message::CUSTOM_HEADER_PREFIX().length();
    std::unordered_map<std::string, std::string> result;

    for (const auto& headersPair : headers) {
        const std::string& headerName = headersPair.first;

        if (isCustomHeaderKey(headerName)) {
            std::string headerNameWithoutPrefix = headerName.substr(CUSTOM_HEADER_PREFIX_LENGTH);

            result.insert({std::move(headerNameWithoutPrefix), headersPair.second});
        }
    }

    return result;
}

std::unordered_map<std::string, std::string> ImmutableMessage::getPrefixedCustomHeaders() const
{
    if (headers.size() <= RequiredHeaders::NUM_REQUIRED_HEADERS) {
        return std::unordered_map<std::string, std::string>();
    }

    std::unordered_map<std::string, std::string> result;

    for (const auto& headersPair : headers) {
        const std::string& headerName = headersPair.first;

        if (isCustomHeaderKey(headerName)) {
            result.insert({headersPair.first, headersPair.second});
        }
    }

    return result;
}

bool ImmutableMessage::isEncrypted() const
{
    return _messageDeserializer.isEncrypted();
}

bool ImmutableMessage::isSigned() const
{
    return _messageDeserializer.isSigned();
}

bool ImmutableMessage::isCompressed() const
{
    return _messageDeserializer.isCompressed();
}

smrf::ByteArrayView ImmutableMessage::getUnencryptedBody() const
{
    if (!_bodyView) {
        if (!_messageDeserializer.isCompressed()) {
            _bodyView = _messageDeserializer.getBody();
        } else {
            _decompressedBody = _messageDeserializer.decompressBody();
            _bodyView = smrf::ByteArrayView(*_decompressedBody);
        }
    }
    return *_bodyView;
}

std::string ImmutableMessage::toLogMessage() const
{
    return serializer::serializeToJson(*this);
}

const std::string& ImmutableMessage::getType() const
{
    return _requiredHeaders.type;
}

const std::string& ImmutableMessage::getId() const
{
    return _requiredHeaders.id;
}

boost::optional<std::string> ImmutableMessage::getReplyTo() const
{
    return getOptionalHeaderByKey(Message::HEADER_REPLY_TO());
}

boost::optional<std::string> ImmutableMessage::getEffort() const
{
    return getOptionalHeaderByKey(Message::HEADER_EFFORT());
}

TimePoint ImmutableMessage::getExpiryDate() const
{
    // for now we only support absolute TTLs
    assert(_messageDeserializer.isTtlAbsolute());
    return TimePoint::fromAbsoluteMs(_messageDeserializer.getTtlMs());
}

const smrf::ByteVector& ImmutableMessage::getSerializedMessage() const
{
    return _serializedMessage;
}

std::size_t ImmutableMessage::getMessageSize() const
{
    return _messageDeserializer.getMessageSize();
}

smrf::ByteArrayView ImmutableMessage::getSignature() const
{
    return _messageDeserializer.getSignature();
}

bool ImmutableMessage::isReceivedFromGlobal() const
{
    return receivedFromGlobal;
}

void ImmutableMessage::setReceivedFromGlobal(bool recFromGlobal)
{
    this->receivedFromGlobal = recFromGlobal;
}

void ImmutableMessage::setCreator(const std::string& creatorLocal)
{
    this->creator = creatorLocal;
}

void ImmutableMessage::setCreator(std::string&& creatorLocal)
{
    this->creator = std::move(creatorLocal);
}

const std::string& ImmutableMessage::getCreator() const
{
    return creator;
}

boost::optional<std::string> ImmutableMessage::getOptionalHeaderByKey(const std::string& key) const
{
    boost::optional<std::string> value;
    auto it = headers.find(key);
    if (it != headers.cend()) {
        value = it->second;
    }
    return value;
}

void ImmutableMessage::init()
{
    headers = _messageDeserializer.getHeaders();
    boost::optional<std::string> optionalId = getOptionalHeaderByKey(Message::HEADER_ID());
    boost::optional<std::string> optionalType = getOptionalHeaderByKey(Message::HEADER_TYPE());

    JOYNR_LOG_TRACE(logger(), "init: {}", toLogMessage());

    // check if necessary headers are set
    if (!optionalId.is_initialized() || !optionalType.is_initialized()) {
        throw std::invalid_argument("missing header");
    } else {
        _requiredHeaders.id = std::move(*optionalId);
        _requiredHeaders.type = std::move(*optionalType);
    }
}

bool ImmutableMessage::isCustomHeaderKey(const std::string& key) const
{
    return boost::algorithm::starts_with(key, Message::CUSTOM_HEADER_PREFIX());
}

bool ImmutableMessage::isAccessControlChecked() const
{
    return _accessControlChecked;
}

void ImmutableMessage::setAccessControlChecked()
{
    _accessControlChecked = true;
}

std::string ImmutableMessage::getTrackingInfo() const
{
    auto requestReplyId = getOptionalHeaderByKey(Message::CUSTOM_HEADER_PREFIX() +
                                                 Message::CUSTOM_HEADER_REQUEST_REPLY_ID());
    std::string trackingInfo = "messageId: " + getId() + ", type: " + getType() + ", sender: " +
                               getSender() + ", recipient: " + getRecipient() +
                               (requestReplyId ? ", requestReplyId: " + *requestReplyId : "") +
                               ", expiryDate: " + std::to_string(getExpiryDate().toMilliseconds()) +
                               ", size: " + std::to_string(getMessageSize());
    return trackingInfo;
}

} // namespace joynr

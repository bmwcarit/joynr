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

#include "boost/algorithm/string.hpp"

#include "joynr/Message.h"

namespace joynr
{

INIT_LOGGER(ImmutableMessage);

ImmutableMessage::ImmutableMessage(smrf::ByteVector&& serializedMessage, bool verifyInput)
        : serializedMessage(std::move(serializedMessage)),
          messageDeserializer(smrf::ByteArrayView(this->serializedMessage), verifyInput),
          headers(),
          bodyView(),
          decompressedBody(),
          receivedFromGlobal(false),
          creator(),
          requiredHeaders()
{
    init();
}

ImmutableMessage::ImmutableMessage(const smrf::ByteVector& serializedMessage, bool verifyInput)
        : serializedMessage(serializedMessage),
          messageDeserializer(smrf::ByteArrayView(this->serializedMessage), verifyInput),
          headers(),
          bodyView(),
          decompressedBody(),
          receivedFromGlobal(false),
          creator(),
          requiredHeaders()
{
    init();
}

std::string ImmutableMessage::getSender() const
{
    return messageDeserializer.getSender();
}

std::string ImmutableMessage::getRecipient() const
{
    return messageDeserializer.getRecipient();
}

bool ImmutableMessage::isTtlAbsolute() const
{
    return messageDeserializer.isTtlAbsolute();
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
    return messageDeserializer.isEncrypted();
}

bool ImmutableMessage::isSigned() const
{
    return messageDeserializer.isSigned();
}

smrf::ByteArrayView ImmutableMessage::getUnencryptedBody() const
{
    if (!bodyView) {
        if (!messageDeserializer.isCompressed()) {
            bodyView = messageDeserializer.getBody();
        } else {
            decompressedBody = messageDeserializer.decompressBody();
            bodyView = smrf::ByteArrayView(*decompressedBody);
        }
    }
    return *bodyView;
}

std::string ImmutableMessage::toLogMessage() const
{
    return serializer::serializeToJson(*this);
}

const std::string& ImmutableMessage::getType() const
{
    return requiredHeaders.type;
}

const std::string& ImmutableMessage::getId() const
{
    return requiredHeaders.id;
}

boost::optional<std::string> ImmutableMessage::getReplyTo() const
{
    return getOptionalHeaderByKey(Message::HEADER_REPLY_TO());
}

boost::optional<std::string> ImmutableMessage::getEffort() const
{
    return getOptionalHeaderByKey(Message::HEADER_EFFORT());
}

JoynrTimePoint ImmutableMessage::getExpiryDate() const
{
    // for now we only support absolute TTLs
    assert(messageDeserializer.isTtlAbsolute());
    return JoynrTimePoint(std::chrono::milliseconds(messageDeserializer.getTtlMs()));
}

const smrf::ByteVector& ImmutableMessage::getSerializedMessage() const
{
    return serializedMessage;
}

std::size_t ImmutableMessage::getMessageSize() const
{
    return messageDeserializer.getMessageSize();
}

smrf::ByteArrayView ImmutableMessage::getSignature() const
{
    return messageDeserializer.getSignature();
}

bool ImmutableMessage::isReceivedFromGlobal() const
{
    return receivedFromGlobal;
}

void ImmutableMessage::setReceivedFromGlobal(bool receivedFromGlobal)
{
    this->receivedFromGlobal = receivedFromGlobal;
}

void ImmutableMessage::setCreator(const std::string& creator)
{
    this->creator = creator;
}

void ImmutableMessage::setCreator(std::string&& creator)
{
    this->creator = std::move(creator);
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
    headers = messageDeserializer.getHeaders();
    boost::optional<std::string> optionalId = getOptionalHeaderByKey(Message::HEADER_ID());
    boost::optional<std::string> optionalType = getOptionalHeaderByKey(Message::HEADER_TYPE());

    JOYNR_LOG_TRACE(logger, "init: {}", toLogMessage());

    // check if necessary headers are set
    if (!optionalId.is_initialized() || !optionalType.is_initialized()) {
        throw std::invalid_argument("missing header");
    } else {
        requiredHeaders.id = std::move(*optionalId);
        requiredHeaders.type = std::move(*optionalType);
    }
}

bool ImmutableMessage::isCustomHeaderKey(const std::string& key) const
{
    return boost::algorithm::starts_with(key, Message::CUSTOM_HEADER_PREFIX());
}

} // namespace joynr

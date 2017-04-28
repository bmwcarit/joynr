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
#include "joynr/MessagingQos.h"

#include <exception>
#include <sstream>
#include <regex>

namespace joynr
{

MessagingQos::MessagingQos(std::uint64_t ttl,
                           MessagingQosEffort::Enum effort,
                           bool encrypt,
                           bool compress)
        : ttl(ttl), effort(effort), encrypt(encrypt), compress(compress), messageHeaders()
{
}

MessagingQos::MessagingQos(MessagingQosEffort::Enum effort, bool encrypt)
        : MessagingQos::MessagingQos(default_ttl, effort, encrypt, false)
{
}

MessagingQos::MessagingQos(std::uint64_t ttl, bool encrypt)
        : MessagingQos::MessagingQos(ttl, MessagingQosEffort::Enum::NORMAL, encrypt, false)
{
}

std::uint64_t MessagingQos::getTtl() const
{
    return ttl;
}

void MessagingQos::setTtl(const std::uint64_t& ttl)
{
    this->ttl = ttl;
}

MessagingQosEffort::Enum MessagingQos::getEffort() const
{
    return effort;
}

void MessagingQos::setEffort(const MessagingQosEffort::Enum effort)
{
    this->effort = effort;
}

bool MessagingQos::getEncrypt() const
{
    return encrypt;
}

void MessagingQos::setEncrypt(const bool encrypt)
{
    this->encrypt = encrypt;
}

bool MessagingQos::getCompress() const
{
    return compress;
}

void MessagingQos::setCompress(const bool compress)
{
    this->compress = compress;
}

void MessagingQos::putCustomMessageHeader(const std::string& key, const std::string& value)
{
    checkCustomHeaderKeyValue(key, value);
    messageHeaders[key] = value;
}

void MessagingQos::putAllCustomMessageHeaders(
        const std::unordered_map<std::string, std::string>& values)
{
    for (const auto& it : values) {
        checkCustomHeaderKeyValue(it.first, it.second);
    }
    messageHeaders = values;
}

void MessagingQos::checkCustomHeaderKeyValue(const std::string& key, const std::string& value) const
{
    if (!std::regex_match(key, std::regex(R"(^[a-zA-Z0-9\-]*$)", std::regex::extended))) {
        throw std::invalid_argument("key may only contain alphanumeric characters");
    }
    if (!std::regex_match(
                value, std::regex(R"(^([a-zA-Z0-9 ;:,+&\?\.\*\/\\]|-)*$)", std::regex::extended))) {
        throw std::invalid_argument(
                "value contains illegal character. See API docs for allowed characters");
    }
}

const std::unordered_map<std::string, std::string>& MessagingQos::getCustomMessageHeaders() const
{
    return messageHeaders;
}

bool MessagingQos::operator==(const MessagingQos& other) const
{
    return (this->getTtl() == other.getTtl() && this->getEffort() == other.getEffort() &&
            this->getEncrypt() == other.getEncrypt() &&
            this->getCompress() == other.getCompress() &&
            this->getCustomMessageHeaders() == other.getCustomMessageHeaders());
}

std::string MessagingQos::toString() const
{
    std::ostringstream msgQosAsString;
    msgQosAsString << "MessagingQos{";
    msgQosAsString << "ttl:" << getTtl();
    msgQosAsString << "effort:" << MessagingQosEffort::getLiteral(this->getEffort());
    msgQosAsString << "encrypt:" << this->getEncrypt();
    msgQosAsString << "compress:" << this->getCompress();
    msgQosAsString << "}";
    return msgQosAsString.str();
}

// printing RadioStation with google-test and google-mock
void PrintTo(const MessagingQos& messagingQos, ::std::ostream* os)
{
    *os << "MessagingQos::" << messagingQos.toString();
}

} // namespace joynr

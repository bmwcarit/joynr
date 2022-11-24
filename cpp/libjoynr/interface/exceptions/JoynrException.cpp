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
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

namespace exceptions
{

// TODO This is a workaround which must be removed after the new serializer is introduced
const std::string& JoynrException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrException";
    return TYPE_NAME;
}

const std::string& JoynrRuntimeException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrRuntimeException";
    return TYPE_NAME;
}

const std::string& JoynrConfigurationException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrConfigurationException";
    return TYPE_NAME;
}

const std::string& JoynrTimeOutException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrTimeOutException";
    return TYPE_NAME;
}

const std::string& JoynrMessageNotSentException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrMessageNotSentException";
    return TYPE_NAME;
}

const std::string& JoynrMessageExpiredException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrMessageExpiredException";
    return TYPE_NAME;
}

const std::string& JoynrDelayMessageException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.JoynrDelayMessageException";
    return TYPE_NAME;
}

const std::string& DiscoveryException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "io.joynr.exceptions.DiscoveryException";
    return TYPE_NAME;
}

const std::string& ProviderRuntimeException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "joynr.exceptions.ProviderRuntimeException";
    return TYPE_NAME;
}

const std::string& PublicationMissedException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "joynr.exceptions.PublicationMissedException";
    return TYPE_NAME;
}

const std::string& ApplicationException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "joynr.exceptions.ApplicationException";
    return TYPE_NAME;
}

JoynrException::JoynrException() noexcept : _message()
{
}

JoynrException::JoynrException(const std::string& message) noexcept : _message(message)
{
}

const char* JoynrException::what() const noexcept
{
    return _message.is_initialized() ? _message->c_str() : std::exception::what();
}

std::string JoynrException::getMessage() const noexcept
{
    return _message.is_initialized() ? *_message : std::string(std::exception::what());
}

void JoynrException::setMessage(const std::string& message)
{
    this->_message = message;
}

const std::string& JoynrException::getTypeName() const
{
    return JoynrException::TYPE_NAME();
}

bool JoynrException::operator==(const JoynrException& other) const
{
    return _message == other.getMessage();
}

JoynrRuntimeException::JoynrRuntimeException(const std::string& message) noexcept
        : JoynrException(message)
{
}

const std::string& JoynrRuntimeException::getTypeName() const
{
    return JoynrRuntimeException::TYPE_NAME();
}

JoynrRuntimeException* JoynrRuntimeException::clone() const
{
    return new JoynrRuntimeException(const_cast<JoynrRuntimeException&>(*this));
}

JoynrConfigurationException::JoynrConfigurationException(const std::string& message) noexcept
        : JoynrRuntimeException(message)
{
}

const std::string& JoynrConfigurationException::getTypeName() const
{
    return JoynrConfigurationException::TYPE_NAME();
}

JoynrConfigurationException* JoynrConfigurationException::clone() const
{
    return new JoynrConfigurationException(getMessage());
}

JoynrTimeOutException::JoynrTimeOutException(const std::string& message) noexcept
        : JoynrRuntimeException(message)
{
}

const std::string& JoynrTimeOutException::getTypeName() const
{
    return JoynrTimeOutException::TYPE_NAME();
}

JoynrTimeOutException* JoynrTimeOutException::clone() const
{
    return new JoynrTimeOutException(const_cast<JoynrTimeOutException&>(*this));
}

JoynrMessageNotSentException::JoynrMessageNotSentException(const std::string& message) noexcept
        : JoynrRuntimeException(message)
{
}

const std::string& JoynrMessageNotSentException::getTypeName() const
{
    return JoynrMessageNotSentException::TYPE_NAME();
}

JoynrMessageNotSentException* JoynrMessageNotSentException::clone() const
{
    return new JoynrMessageNotSentException(const_cast<JoynrMessageNotSentException&>(*this));
}

JoynrMessageExpiredException::JoynrMessageExpiredException(const std::string& message) noexcept
        : JoynrMessageNotSentException(message)
{
}

const std::string& JoynrMessageExpiredException::getTypeName() const
{
    return JoynrMessageExpiredException::TYPE_NAME();
}

JoynrMessageExpiredException* JoynrMessageExpiredException::clone() const
{
    return new JoynrMessageExpiredException(const_cast<JoynrMessageExpiredException&>(*this));
}

const std::chrono::milliseconds JoynrDelayMessageException::DEFAULT_DELAY_MS(1000);

JoynrDelayMessageException::JoynrDelayMessageException() noexcept
        : JoynrRuntimeException(), _delayMs(DEFAULT_DELAY_MS)
{
}

JoynrDelayMessageException::JoynrDelayMessageException(const std::string& message) noexcept
        : JoynrRuntimeException(message), _delayMs(DEFAULT_DELAY_MS)
{
}

JoynrDelayMessageException::JoynrDelayMessageException(const std::chrono::milliseconds delayMs,
                                                       const std::string& message) noexcept
        : JoynrRuntimeException(message), _delayMs(delayMs)
{
}

std::chrono::milliseconds JoynrDelayMessageException::getDelayMs() const noexcept
{
    return _delayMs;
}

void JoynrDelayMessageException::setDelayMs(const std::chrono::milliseconds& delayMs) noexcept
{
    this->_delayMs = delayMs;
}

const std::string& JoynrDelayMessageException::getTypeName() const
{
    return JoynrDelayMessageException::TYPE_NAME();
}

JoynrDelayMessageException* JoynrDelayMessageException::clone() const
{
    return new JoynrDelayMessageException(const_cast<JoynrDelayMessageException&>(*this));
}

bool JoynrDelayMessageException::operator==(const JoynrDelayMessageException& other) const
{
    return _message == other.getMessage() && _delayMs == other.getDelayMs();
}

JoynrParseError::JoynrParseError(const std::string& message) noexcept
        : JoynrRuntimeException(message)
{
}

DiscoveryException::DiscoveryException(const std::string& message) noexcept
        : JoynrRuntimeException(message)
{
}

const std::string& DiscoveryException::getTypeName() const
{
    return DiscoveryException::TYPE_NAME();
}

DiscoveryException* DiscoveryException::clone() const
{
    return new DiscoveryException(const_cast<DiscoveryException&>(*this));
}

ProviderRuntimeException::ProviderRuntimeException(const std::string& message) noexcept
        : JoynrRuntimeException(message)
{
}

const std::string& ProviderRuntimeException::getTypeName() const
{
    return ProviderRuntimeException::TYPE_NAME();
}

ProviderRuntimeException* ProviderRuntimeException::clone() const
{
    return new ProviderRuntimeException(const_cast<ProviderRuntimeException&>(*this));
}

PublicationMissedException::PublicationMissedException() noexcept
        : JoynrRuntimeException(), _subscriptionId()
{
}

PublicationMissedException::PublicationMissedException(const std::string& subscriptionId) noexcept
        : JoynrRuntimeException(subscriptionId), _subscriptionId(subscriptionId)
{
}

std::string PublicationMissedException::getSubscriptionId() const noexcept
{
    return _subscriptionId;
}

const std::string& PublicationMissedException::getTypeName() const
{
    return PublicationMissedException::TYPE_NAME();
}

void PublicationMissedException::setSubscriptionId(const std::string& newValue) noexcept
{
    _subscriptionId = newValue;
    setMessage(newValue);
}

PublicationMissedException* PublicationMissedException::clone() const
{
    return new PublicationMissedException(const_cast<PublicationMissedException&>(*this));
}

bool PublicationMissedException::operator==(const PublicationMissedException& other) const
{
    return _message == other.getMessage() && _subscriptionId == other.getSubscriptionId();
}

ApplicationException::ApplicationException() noexcept : JoynrException(), _error()
{
}

ApplicationException::ApplicationException(
        const std::string& message,
        std::shared_ptr<ApplicationExceptionError> error) noexcept
        : JoynrException(message), _error(std::move(error))
{
}

std::string ApplicationException::getName() const noexcept
{
    return _error->getName();
}

const std::string& ApplicationException::getTypeName() const
{
    return ApplicationException::TYPE_NAME();
}

ApplicationException* ApplicationException::clone() const
{
    return new ApplicationException(const_cast<ApplicationException&>(*this));
}

bool ApplicationException::operator==(const ApplicationException& other) const
{
    const ApplicationExceptionError* const errorPtr = _error.get();
    const ApplicationExceptionError* const otherErrorPtr = other._error.get();
    return typeid(*errorPtr) == typeid(*(otherErrorPtr)) && _message == other.getMessage() &&
           _error->getName() == other._error->getName();
}

} // namespace exceptions

} // namespace joynr

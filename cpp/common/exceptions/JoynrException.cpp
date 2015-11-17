/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
const std::string JoynrException::TYPE_NAME = "io.joynr.exceptions.JoynrException";
const std::string JoynrRuntimeException::TYPE_NAME = "io.joynr.exceptions.JoynrRuntimeException";
const std::string JoynrTimeOutException::TYPE_NAME = "io.joynr.exceptions.JoynrTimeoutException";
const std::string DiscoveryException::TYPE_NAME = "io.joynr.exceptions.DiscoveryException";
const std::string MethodInvocationException::TYPE_NAME =
        "joynr.exceptions.MethodInvocationException";
const std::string ProviderRuntimeException::TYPE_NAME = "joynr.exceptions.ProviderRuntimeException";
const std::string PublicationMissedException::TYPE_NAME =
        "joynr.exceptions.PublicationMissedException";
const std::string ApplicationException::TYPE_NAME = "joynr.exceptions.ApplicationException";

JoynrException::JoynrException() throw() : message("")
{
}

JoynrException::JoynrException(const std::string& message) throw() : message(message)
{
}

JoynrException::JoynrException(const JoynrException& other) throw() : message(other.message)
{
}

JoynrException::~JoynrException() throw()
{
}

const char* JoynrException::what() const throw()
{
    return message.c_str();
}

const std::string JoynrException::getMessage() const throw()
{
    return message;
}

void JoynrException::setMessage(std::string message)
{
    this->message = message;
}

const std::string JoynrException::getTypeName() const
{
    return JoynrException::TYPE_NAME;
}

JoynrException* JoynrException::clone() const
{
    return new JoynrException(static_cast<JoynrException>(*this));
}

JoynrRuntimeException::JoynrRuntimeException(const std::string& message) throw()
        : JoynrException(message)
{
}

const std::string JoynrRuntimeException::getTypeName() const
{
    return JoynrRuntimeException::TYPE_NAME;
}

JoynrRuntimeException* JoynrRuntimeException::clone() const
{
    return new JoynrRuntimeException(static_cast<JoynrRuntimeException>(*this));
}

JoynrTimeOutException::JoynrTimeOutException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

const std::string JoynrTimeOutException::getTypeName() const
{
    return JoynrTimeOutException::TYPE_NAME;
}

JoynrTimeOutException* JoynrTimeOutException::clone() const
{
    return new JoynrTimeOutException(static_cast<JoynrTimeOutException>(*this));
}

JoynrParseError::JoynrParseError(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

DiscoveryException::DiscoveryException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

const std::string DiscoveryException::getTypeName() const
{
    return DiscoveryException::TYPE_NAME;
}

DiscoveryException* DiscoveryException::clone() const
{
    return new DiscoveryException(static_cast<DiscoveryException>(*this));
}

MethodInvocationException::MethodInvocationException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

const std::string MethodInvocationException::getTypeName() const
{
    return MethodInvocationException::TYPE_NAME;
}

MethodInvocationException* MethodInvocationException::clone() const
{
    return new MethodInvocationException(static_cast<MethodInvocationException>(*this));
}

ProviderRuntimeException::ProviderRuntimeException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

const std::string ProviderRuntimeException::getTypeName() const
{
    return ProviderRuntimeException::TYPE_NAME;
}

ProviderRuntimeException* ProviderRuntimeException::clone() const
{
    return new ProviderRuntimeException(static_cast<ProviderRuntimeException>(*this));
}

PublicationMissedException::PublicationMissedException(const std::string& subscriptionId) throw()
        : JoynrRuntimeException(subscriptionId), subscriptionId(subscriptionId)
{
}

PublicationMissedException::PublicationMissedException(
        const PublicationMissedException& other) throw()
        : JoynrRuntimeException(other.subscriptionId), subscriptionId(other.subscriptionId)
{
}

std::string PublicationMissedException::getSubscriptionId() const throw()
{
    return subscriptionId;
}

const std::string PublicationMissedException::getTypeName() const
{
    return PublicationMissedException::TYPE_NAME;
}

PublicationMissedException* PublicationMissedException::clone() const
{
    return new PublicationMissedException(static_cast<PublicationMissedException>(*this));
}

ApplicationException::ApplicationException(const ApplicationException& other) throw()
        : JoynrException(other.message),
          value(other.value),
          name(other.name),
          typeName(other.typeName)
{
}

ApplicationException::ApplicationException(const uint32_t& value,
                                           const std::string& name,
                                           const std::string& typeName) throw()
        : JoynrException(), value(value), name(name), typeName(typeName)
{
}

ApplicationException::ApplicationException(const std::string& message,
                                           const uint32_t& value,
                                           const std::string& name,
                                           const std::string& typeName) throw()
        : JoynrException(message), value(value), name(name), typeName(typeName)
{
}

uint32_t ApplicationException::getError() const throw()
{
    return value;
}

void ApplicationException::setError(const uint32_t& value)
{
    this->value = value;
}

std::string ApplicationException::getName() const throw()
{
    return name;
}

std::string ApplicationException::getErrorTypeName() const throw()
{
    return typeName;
}

const std::string ApplicationException::getTypeName() const
{
    return ApplicationException::TYPE_NAME;
}

ApplicationException* ApplicationException::clone() const
{
    return new ApplicationException(static_cast<ApplicationException>(*this));
}

} // namespace exceptions

} // namespace joynr

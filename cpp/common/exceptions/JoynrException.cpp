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

JoynrRuntimeException::JoynrRuntimeException(const std::string& message) throw()
        : JoynrException(message)
{
}

JoynrTimeOutException::JoynrTimeOutException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

JoynrParseError::JoynrParseError(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

DiscoveryException::DiscoveryException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

MethodInvocationException::MethodInvocationException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
}

ProviderRuntimeException::ProviderRuntimeException(const std::string& message) throw()
        : JoynrRuntimeException(message)
{
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

} // namespace exceptions

} // namespace joynr

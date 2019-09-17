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
#include "joynr/exceptions/MethodInvocationException.h"

namespace joynr
{

namespace exceptions
{

MethodInvocationException::MethodInvocationException() noexcept : JoynrRuntimeException(),
                                                                  _providerVersion()
{
}

MethodInvocationException::MethodInvocationException(const std::string& message) noexcept
        : JoynrRuntimeException(message),
          _providerVersion()
{
}

MethodInvocationException::MethodInvocationException(
        const std::string& message,
        const joynr::types::Version& providerVersion) noexcept : JoynrRuntimeException(message),
                                                                 _providerVersion(providerVersion)
{
}

MethodInvocationException::MethodInvocationException(const MethodInvocationException& other)
        : JoynrRuntimeException(other), _providerVersion(other._providerVersion)
{
}

const joynr::types::Version& MethodInvocationException::getProviderVersion() const
{
    return _providerVersion;
}

void MethodInvocationException::setProviderVersion(const joynr::types::Version& providerVersion)
{
    this->_providerVersion = providerVersion;
}

const std::string& MethodInvocationException::getTypeName() const
{
    return MethodInvocationException::TYPE_NAME();
}

MethodInvocationException* MethodInvocationException::clone() const
{
    return new MethodInvocationException(const_cast<MethodInvocationException&>(*this));
}

const std::string& MethodInvocationException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "joynr.exceptions.MethodInvocationException";
    return TYPE_NAME;
}

bool MethodInvocationException::operator==(const MethodInvocationException& other) const
{
    return _message == other.getMessage() && _providerVersion == other.getProviderVersion();
}

} // namespace exceptions

} // namespace joynr

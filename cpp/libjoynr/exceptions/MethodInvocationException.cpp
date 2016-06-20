/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "joynr/Variant.h"
#include "joynr/types/Version.h"

namespace joynr
{

namespace exceptions
{

// TODO This is a workaround which must be removed after the new serializer is introduced
const std::string MethodInvocationException::TYPE_NAME =
        "joynr.exceptions.MethodInvocationException";

static const bool isMethodInvocationExceptionRegistered =
        Variant::registerType<joynr::exceptions::MethodInvocationException>(
                MethodInvocationException::TYPE_NAME);

MethodInvocationException::MethodInvocationException() noexcept : JoynrRuntimeException(),
                                                                  providerVersion()
{
}

MethodInvocationException::MethodInvocationException(const std::string& message) noexcept
        : JoynrRuntimeException(message),
          providerVersion()
{
}

MethodInvocationException::MethodInvocationException(const std::string& message,
                                                     const Version& providerVersion) noexcept
        : JoynrRuntimeException(message),
          providerVersion(providerVersion)
{
}

const std::string MethodInvocationException::getTypeName() const
{
    return MethodInvocationException::TYPE_NAME;
}

MethodInvocationException* MethodInvocationException::clone() const
{
    return new MethodInvocationException(const_cast<MethodInvocationException&>(*this));
}

} // namespace exceptions

} // namespace joynr

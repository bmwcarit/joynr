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
#ifndef JOYNREXCEPTIONSUTIL_H
#define JOYNREXCEPTIONSUTIL_H

#include <exception>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/exceptions/SubscriptionException.h"

namespace joynr
{

namespace exceptions
{

std::exception_ptr convertToExceptionPtr(const JoynrException& error);

std::exception_ptr createJoynrTimeOutException(const std::string& message);

/**
 * @brief Util class to transforms between Variant and joynr exceptions.
 */
class JOYNR_EXPORT JoynrExceptionUtil
{
public:
    static void throwJoynrException(const exceptions::JoynrException& error)
    {
        const std::string& typeName = error.getTypeName();
        if (typeName == exceptions::JoynrRuntimeException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::JoynrRuntimeException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::JoynrTimeOutException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::JoynrTimeOutException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::DiscoveryException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::DiscoveryException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::MethodInvocationException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::MethodInvocationException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::ProviderRuntimeException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::ProviderRuntimeException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::PublicationMissedException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::PublicationMissedException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::SubscriptionException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::SubscriptionException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::ApplicationException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::ApplicationException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::JoynrMessageNotSentException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::JoynrMessageNotSentException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::JoynrMessageExpiredException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::JoynrMessageExpiredException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else if (typeName == exceptions::JoynrDelayMessageException::TYPE_NAME()) {
            throw dynamic_cast<exceptions::JoynrDelayMessageException&>(
                    const_cast<exceptions::JoynrException&>(error));
        } else {
            throw exceptions::JoynrRuntimeException("Unknown exception: " + error.getTypeName() +
                                                    ": " + error.getMessage());
        }
    }
};

} // namespace exceptions

} // namespace joynr
#endif // JOYNREXCEPTIONSUTIL_H

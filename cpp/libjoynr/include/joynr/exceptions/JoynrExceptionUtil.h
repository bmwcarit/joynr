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
#ifndef JOYNREXCEPTIONSUTIL_H
#define JOYNREXCEPTIONSUTIL_H

#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/Variant.h"
namespace joynr
{

namespace exceptions
{

/**
 * @brief Util class to transforms between Variant and joynr exceptions.
 */
class JOYNRCOMMON_EXPORT JoynrExceptionUtil
{
public:
    static Variant createVariant(const exceptions::JoynrException& exception)
    {
        if (dynamic_cast<const exceptions::ApplicationException*>(&exception) != nullptr) {
            return Variant::make<exceptions::ApplicationException>(
                    static_cast<const exceptions::ApplicationException&>(exception));
        } else if (dynamic_cast<const exceptions::PublicationMissedException*>(&exception) !=
                   nullptr) {
            return Variant::make<exceptions::PublicationMissedException>(
                    static_cast<const exceptions::PublicationMissedException&>(exception));
        } else if (dynamic_cast<const exceptions::ProviderRuntimeException*>(&exception) !=
                   nullptr) {
            return Variant::make<exceptions::ProviderRuntimeException>(
                    static_cast<const exceptions::ProviderRuntimeException&>(exception));
        } else if (dynamic_cast<const exceptions::MethodInvocationException*>(&exception) !=
                   nullptr) {
            return Variant::make<exceptions::MethodInvocationException>(
                    static_cast<const exceptions::MethodInvocationException&>(exception));
        } else if (dynamic_cast<const exceptions::JoynrTimeOutException*>(&exception) != nullptr) {
            return Variant::make<exceptions::JoynrTimeOutException>(
                    static_cast<const exceptions::JoynrTimeOutException&>(exception));
        } else if (dynamic_cast<const exceptions::JoynrParseError*>(&exception) != nullptr) {
            return Variant::make<exceptions::JoynrParseError>(
                    static_cast<const exceptions::JoynrParseError&>(exception));
        } else if (dynamic_cast<const exceptions::DiscoveryException*>(&exception) != nullptr) {
            return Variant::make<exceptions::DiscoveryException>(
                    static_cast<const exceptions::DiscoveryException&>(exception));
        } else if (dynamic_cast<const exceptions::JoynrRuntimeException*>(&exception) != nullptr) {
            return Variant::make<exceptions::JoynrRuntimeException>(
                    static_cast<const exceptions::JoynrRuntimeException&>(exception));
        } else if (dynamic_cast<const exceptions::JoynrMessageNotSentException*>(&exception) !=
                   nullptr) {
            return Variant::make<exceptions::JoynrMessageNotSentException>(
                    static_cast<const exceptions::JoynrMessageNotSentException&>(exception));
        } else if (dynamic_cast<const exceptions::JoynrDelayMessageException*>(&exception) !=
                   nullptr) {
            return Variant::make<exceptions::JoynrDelayMessageException>(
                    static_cast<const exceptions::JoynrDelayMessageException&>(exception));
        }
        return Variant::make<exceptions::JoynrException>(exception);
    }

    static const exceptions::JoynrRuntimeException& extractJoynrRuntimeException(
            const Variant& variant)
    {
        if (variant.is<exceptions::PublicationMissedException>()) {
            return variant.get<exceptions::PublicationMissedException>();
        } else if (variant.is<exceptions::ProviderRuntimeException>()) {
            return variant.get<exceptions::ProviderRuntimeException>();
        } else if (variant.is<exceptions::MethodInvocationException>()) {
            return variant.get<exceptions::MethodInvocationException>();
        } else if (variant.is<exceptions::JoynrTimeOutException>()) {
            return variant.get<exceptions::JoynrTimeOutException>();
        } else if (variant.is<exceptions::JoynrParseError>()) {
            return variant.get<exceptions::JoynrParseError>();
        } else if (variant.is<exceptions::DiscoveryException>()) {
            return variant.get<exceptions::DiscoveryException>();
        } else if (variant.is<exceptions::JoynrRuntimeException>()) {
            return variant.get<exceptions::JoynrRuntimeException>();
        } else if (variant.is<exceptions::JoynrMessageNotSentException>()) {
            return variant.get<exceptions::JoynrMessageNotSentException>();
        } else if (variant.is<exceptions::JoynrDelayMessageException>()) {
            return variant.get<exceptions::JoynrDelayMessageException>();
        } else
            throw exceptions::JoynrRuntimeException(
                    "Exception type contained in Variant with typeName " + variant.getTypeName());
    }

    static const exceptions::JoynrException& extractException(const Variant& variant)
    {
        if (variant.is<exceptions::ApplicationException>()) {
            return variant.get<exceptions::ApplicationException>();
        } else if (variant.is<exceptions::JoynrException>()) {
            return variant.get<exceptions::JoynrException>();
        } else {
            return extractJoynrRuntimeException(variant);
        }
    }
};

} // namespace exceptions

} // namespace joynr
#endif // JOYNREXCEPTIONSUTIL_H

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
#ifndef METHODINVOCATIONEXCEPTION_H
#define METHODINVOCATIONEXCEPTION_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/types/Version.h"

namespace joynr
{
namespace exceptions
{

/**
 * @brief Joynr exception class to report error during method invocations (RPC) at a provider
 * ("no such method", invalid arguments, etc.)
 */
class JOYNR_EXPORT MethodInvocationException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a MethodInvocationException without detail message.
     */
    MethodInvocationException() noexcept;

    /**
     * @brief Copy Constructor
     *
     * @param other The PublicationMissedException to copy from.
     */
    MethodInvocationException(const MethodInvocationException& other);

    /**
     * @brief Constructor for a MethodInvocationException with detail message.
     *
     * @param message Further description of the reported invocation error
     */
    explicit MethodInvocationException(const std::string& message) noexcept;
    explicit MethodInvocationException(const std::string& message,
                                       const joynr::types::Version& providerVersion) noexcept;
    const std::string& getTypeName() const override;
    MethodInvocationException* clone() const override;

    void setProviderVersion(const joynr::types::Version& providerVersion);
    const joynr::types::Version& getProviderVersion() const;

    /**
     * Equality operator
     */
    bool operator==(const MethodInvocationException& other) const;

    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::BaseClass<JoynrRuntimeException>(this),
           muesli::make_nvp("providerVersion", _providerVersion));
    }

private:
    joynr::types::Version _providerVersion;
};

} // namespace exceptions
} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::MethodInvocationException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.MethodInvocationException")
#endif // METHODINVOCATIONEXCEPTION_H

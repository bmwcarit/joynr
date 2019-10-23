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
#ifndef SUBSCRIPTIONEXCEPTION_H
#define SUBSCRIPTIONEXCEPTION_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{
namespace exceptions
{

/**
 * @brief Joynr exception class for reporting error conditions when creating a
 * subscription (e.g. the provided subscription parameters are not correct etc.)
 * that should be transmitted back to consumer side.
 */
class JOYNR_EXPORT SubscriptionException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a SubscriptionException without detail message.
     */
    SubscriptionException() noexcept;

    /**
     * @brief Copy Constructor
     *
     * @param other The SubscriptionException to copy from.
     */
    SubscriptionException(const SubscriptionException& other);

    /**
     * @brief Constructor for a SubscriptionException with detail message and subscriptionId.
     *
     * @param message Further description of the reported invocation error
     * @param subscriptionId the subscriptionId of the subscription which could not be created
     */
    explicit SubscriptionException(const std::string& message,
                                   const std::string& subscriptionIdLocal) noexcept;
    /**
     * @brief Constructor for a SubscriptionException with subscriptionId.
     *
     * @param subscriptionId the subscriptionId of the subscription which could not be created
     */
    explicit SubscriptionException(const std::string& subscriptionIdLocal) noexcept;
    const std::string& getTypeName() const override;
    SubscriptionException* clone() const override;

    void setSubscriptionId(const std::string& subscriptionIdLocal);
    const std::string& getSubscriptionId() const;

    /**
     * Equality operator
     */
    bool operator==(const SubscriptionException& other) const;

    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::BaseClass<JoynrRuntimeException>(this), MUESLI_NVP(subscriptionId));
    }

private:
    std::string subscriptionId;
};

} // namespace exceptions
} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::SubscriptionException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.SubscriptionException")
#endif // SUBSCRIPTIONEXCEPTION_H

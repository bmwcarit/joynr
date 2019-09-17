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
#include "joynr/exceptions/SubscriptionException.h"

namespace joynr
{

namespace exceptions
{

SubscriptionException::SubscriptionException() noexcept : JoynrRuntimeException(), _subscriptionId()
{
}

SubscriptionException::SubscriptionException(const std::string& message,
                                             const std::string& subscriptionId) noexcept
        : JoynrRuntimeException(message),
          _subscriptionId(subscriptionId)
{
}

SubscriptionException::SubscriptionException(const std::string& subscriptionId) noexcept
        : JoynrRuntimeException(subscriptionId),
          _subscriptionId(subscriptionId)
{
}

SubscriptionException::SubscriptionException(const SubscriptionException& other)
        : JoynrRuntimeException(other), _subscriptionId(other._subscriptionId)
{
}

const std::string& SubscriptionException::getSubscriptionId() const
{
    return _subscriptionId;
}

void SubscriptionException::setSubscriptionId(const std::string& subscriptionId)
{
    this->_subscriptionId = subscriptionId;
}

const std::string& SubscriptionException::getTypeName() const
{
    return SubscriptionException::TYPE_NAME();
}

SubscriptionException* SubscriptionException::clone() const
{
    return new SubscriptionException(const_cast<SubscriptionException&>(*this));
}

const std::string& SubscriptionException::TYPE_NAME()
{
    static const std::string TYPE_NAME = "joynr.exceptions.SubscriptionException";
    return TYPE_NAME;
}

bool SubscriptionException::operator==(const SubscriptionException& other) const
{
    return _message == other.getMessage() && _subscriptionId == other.getSubscriptionId();
}

} // namespace exceptions

} // namespace joynr

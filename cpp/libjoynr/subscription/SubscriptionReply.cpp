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

#include "joynr/SubscriptionReply.h"

namespace joynr
{

SubscriptionReply::SubscriptionReply() : _subscriptionId(), _error(nullptr)
{
}

SubscriptionReply::SubscriptionReply(const SubscriptionReply& other)
        : _subscriptionId(other.getSubscriptionId()), _error(other.getError())
{
}

SubscriptionReply& SubscriptionReply::operator=(const SubscriptionReply& other)
{
    this->_subscriptionId = other.getSubscriptionId();
    return *this;
}

std::string SubscriptionReply::toString() const
{
    std::ostringstream typeAsString;
    typeAsString << "SubscriptionReply{";
    typeAsString << "subscriptionId:" + _subscriptionId;
    if (_error) {
        typeAsString << ", ";
        typeAsString << "error: SubscriptionException: " + _error->getMessage();
    }
    typeAsString << "}";
    return typeAsString.str();
}

// printing SubscriptionReply with google-test and google-mock
void PrintTo(const SubscriptionReply& subscriptionReply, ::std::ostream* os)
{
    *os << "SubscriptionReply::" << subscriptionReply.toString();
}

const std::string& SubscriptionReply::getSubscriptionId() const
{
    return _subscriptionId;
}

void SubscriptionReply::setSubscriptionId(const std::string& subscriptionId)
{
    this->_subscriptionId = subscriptionId;
}

void SubscriptionReply::setSubscriptionId(std::string&& subscriptionId)
{
    this->_subscriptionId = std::move(subscriptionId);
}

std::shared_ptr<exceptions::SubscriptionException> SubscriptionReply::getError() const
{
    return _error;
}

void SubscriptionReply::setError(std::shared_ptr<exceptions::SubscriptionException> error)
{
    this->_error = std::move(error);
}

bool SubscriptionReply::operator==(const SubscriptionReply& other) const
{
    // if error ptr do not point to the same object
    if (_error != other.getError()) {
        // if exactly one of error and other.getError() is a nullptr
        if (_error == nullptr || other.getError() == nullptr) {
            return false;
        }
        // compare actual objects
        if (!(*_error.get() == *other.getError().get())) {
            return false;
        }
    }

    return _subscriptionId == other.getSubscriptionId();
}

bool SubscriptionReply::operator!=(const SubscriptionReply& other) const
{
    return !(*this == other);
}

} // namespace joynr

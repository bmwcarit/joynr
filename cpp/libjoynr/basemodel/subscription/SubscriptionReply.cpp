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

SubscriptionReply::SubscriptionReply() : subscriptionId(), error(nullptr)
{
}

SubscriptionReply::SubscriptionReply(const SubscriptionReply& other)
        : subscriptionId(other.getSubscriptionId()), error(other.getError())
{
}

SubscriptionReply& SubscriptionReply::operator=(const SubscriptionReply& other)
{
    this->subscriptionId = other.getSubscriptionId();
    return *this;
}

std::string SubscriptionReply::toString() const
{
    std::ostringstream typeAsString;
    typeAsString << "SubscriptionReply{";
    typeAsString << "subscriptionId:" + subscriptionId;
    if (error) {
        typeAsString << ", ";
        typeAsString << "error: SubscriptionException: " + error->getMessage();
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
    return subscriptionId;
}

void SubscriptionReply::setSubscriptionId(const std::string& subscriptionIdLocal)
{
    this->subscriptionId = subscriptionIdLocal;
}

void SubscriptionReply::setSubscriptionId(std::string&& subscriptionIdLocal)
{
    this->subscriptionId = std::move(subscriptionIdLocal);
}

std::shared_ptr<exceptions::SubscriptionException> SubscriptionReply::getError() const
{
    return error;
}

void SubscriptionReply::setError(std::shared_ptr<exceptions::SubscriptionException> errorLocal)
{
    this->error = std::move(errorLocal);
}

bool SubscriptionReply::operator==(const SubscriptionReply& other) const
{
    // if error ptr do not point to the same object
    if (error != other.getError()) {
        // if exactly one of error and other.getError() is a nullptr
        if (error == nullptr || other.getError() == nullptr) {
            return false;
        }
        // compare actual objects
        if (!(*error.get() == *other.getError().get())) {
            return false;
        }
    }

    return subscriptionId == other.getSubscriptionId();
}

bool SubscriptionReply::operator!=(const SubscriptionReply& other) const
{
    return !(*this == other);
}

} // namespace joynr
